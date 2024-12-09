MAJOR_RELEASE = 2
MINOR_RELEASE = 2

default:

docs: doxy notes

doxy:
	cd docs; \
	doxygen Doxyfile;
	cd docs/latex; \
	make pdf; \
	cp refman.pdf ../TrickCFS.pdf

notes:
	echo "Changes since TrickCFS v$(MAJOR_RELEASE).$(MINOR_RELEASE).0" > docs/Release-Notes.txt
	git log --pretty=format:" - %s" --no-merges $(MAJOR_RELEASE).$(MINOR_RELEASE).0..HEAD >> docs/Release-Notes.txt
