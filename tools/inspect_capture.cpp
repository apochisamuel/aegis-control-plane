#include "aegis/aegis.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
int main(int argc,char**argv){ if(argc!=2){std::cerr<<"usage: inspect_capture FILE\n";return 2;} std::ifstream f(argv[1],std::ios::binary);std::vector<std::uint8_t>b((std::istreambuf_iterator<char>(f)),{});try{aegis::Engine e;auto r=e.ingest(b);std::cout<<r.health<<"\n";for(const auto&s:r.observations)std::cout<<s<<"\n";return r.rejected?1:0;}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
