
check: example
	# run html-tidy to check for errors http://www.html-tidy.org/
	find ./build/public *.html -printf '%p' -exec tidy -eq {} \;

serve: prep_example
	# build the example site and serve
	cd build && hugo serve

example: prep_example
	# build the example
	cd build && hugo

prep_example: clean
	# prepare the example site
	mkdir -p build/themes
	cp -r exampleSite/* build
	cd build/themes && ln -s ../../ hugo-dusk

clean:
	rm -rf build
