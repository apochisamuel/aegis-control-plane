#include "aegis/domain.hpp"
#include "aegis/bytes.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
namespace aegis {
void SubscriberTable::upsert(Subscriber v){ auto it=values_.find(v.id); if(it!=values_.end()&&it->second.generation>v.generation) return; values_[v.id]=std::move(v); }
std::optional<Subscriber> SubscriberTable::find(std::uint32_t id) const { auto it=values_.find(id); if(it==values_.end())return {};return it->second; }
std::vector<Subscriber> SubscriberTable::values() const { std::vector<Subscriber> out;for(const auto& [_,v]:values_)out.push_back(v);return out; }
void RouteCache::upsert(RouteRecord v){ auto old=index_.find(v.id); if(old!=index_.end()){ old->second->generation=v.generation;old->second->destination=std::move(v.destination);old->second->weight=v.weight;old->second->enabled=v.enabled;return;} storage_.push_back(std::make_unique<RouteRecord>(std::move(v)));index_[storage_.back()->id]=storage_.back().get(); }
RouteRecord* RouteCache::find(std::uint32_t id){auto it=index_.find(id);return it==index_.end()?nullptr:it->second;}
const RouteRecord* RouteCache::find(std::uint32_t id)const{auto it=index_.find(id);return it==index_.end()?nullptr:it->second;}
void RouteCache::compact(std::uint32_t min){ std::vector<std::unique_ptr<RouteRecord>> rebuilt; rebuilt.reserve(storage_.size()); for(const auto& route:storage_) if(route->enabled&&route->generation>=min) rebuilt.push_back(std::make_unique<RouteRecord>(*route)); storage_.swap(rebuilt); index_.clear(); for(const auto& route:storage_) index_[route->id]=route.get(); }
std::vector<RouteRecord> RouteCache::values()const{std::vector<RouteRecord>out;for(const auto& r:storage_)out.push_back(*r);return out;}
void PolicyStore::stage(PolicyRule r){auto it=staged_.find(r.id);if(it!=staged_.end()&&it->second.revision>r.revision)return;staged_[r.id]=std::move(r);}
std::optional<PolicyRule> PolicyStore::staged(std::uint32_t id)const{auto it=staged_.find(id);if(it==staged_.end())return {};return it->second;}
std::optional<PolicyRule> PolicyStore::activate(std::uint32_t id){auto it=staged_.find(id);if(it==staged_.end())return {};active_[id]=it->second;staged_.erase(it);return active_[id];}
std::optional<PolicyRule> PolicyStore::active(std::uint32_t id)const{auto it=active_.find(id);if(it==active_.end())return {};return it->second;}
std::vector<PolicyRule> PolicyStore::values()const{std::vector<PolicyRule>out;for(const auto&[_,r]:active_)out.push_back(r);return out;}
void SessionStore::upsert(Session v){values_[v.id]=std::move(v);}Session* SessionStore::find(std::uint32_t id){auto it=values_.find(id);return it==values_.end()?nullptr:&it->second;}const Session* SessionStore::find(std::uint32_t id)const{auto it=values_.find(id);return it==values_.end()?nullptr:&it->second;}void SessionStore::erase(std::uint32_t id){values_.erase(id);}std::vector<Session> SessionStore::values()const{std::vector<Session>out;for(const auto&[_,v]:values_)out.push_back(v);return out;}
void DispatchBook::put(DispatchPlan p){plans_[p.session_id()]=std::move(p);}DispatchPlan* DispatchBook::find(std::uint32_t id){auto it=plans_.find(id);return it==plans_.end()?nullptr:&it->second;}const DispatchPlan* DispatchBook::find(std::uint32_t id)const{auto it=plans_.find(id);return it==plans_.end()?nullptr:&it->second;}void DispatchBook::erase(std::uint32_t id){plans_.erase(id);}std::vector<DispatchPlan> DispatchBook::values()const{std::vector<DispatchPlan>out;for(const auto&[_,v]:plans_)out.push_back(v);return out;}
void ExtensionStore::add(Extension e){const auto sid=e.session_id;values_.emplace(sid,std::move(e));++revisions_[sid];}
std::vector<Extension> ExtensionStore::for_session(std::uint32_t id)const{std::vector<Extension>out;auto[a,b]=values_.equal_range(id);for(;a!=b;++a)out.push_back(a->second);return out;}
std::uint64_t ExtensionStore::revision(std::uint32_t id)const{auto found=revisions_.find(id);return found==revisions_.end()?0:found->second;}
void AuditJournal::append(std::string source,std::string message){entries_.push_back({entries_.size()+1,std::move(source),std::move(message)});}std::vector<AuditEvent> AuditJournal::tail(std::size_t n)const{if(n>=entries_.size())return entries_;return{entries_.end()-static_cast<std::ptrdiff_t>(n),entries_.end()};}
}
