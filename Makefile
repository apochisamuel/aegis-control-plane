.PHONY: build test clean
build:
	cmake -S . -B build -DAEGIS_BUILD_TESTS=ON -DAEGIS_BUILD_TOOLS=ON
	cmake --build build -j

test: build
	ctest --test-dir build --output-on-failure

clean:
	rm -rf build
