
all dist:
	gem build razorback.gemspec

clean distclean mostlyclean maintainer-clean:
	( for I in *.gem; do rm $$I; done )

install:
	gem install razorback-0.4.1.gem	

EMPTY_AUTOMAKE_TARGETS = dvi pdf ps info html tags ctags installdirs install-data install-exec uninstall install-dvi install-html install-infi install-ps install-pdf check distdir
.PHONY: $(EMPTY_AUTOMAKE_TARGETS)
$(EMPTY_AUTOMAKE_TARGETS):
