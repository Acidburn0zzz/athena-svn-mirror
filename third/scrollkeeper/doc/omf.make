# THIS VERSION IS CUSTOMIZED FOR SCROLLKEEPER.  DO NOT USE IN OTHER PACKAGES
# THAN SCROLLKEEPER ITSELF.

# This file contains the build instructions for installing OMF files.  It is
# generally called from the makefiles for particular formats of documentation.
#
# Note: This make file is not incorporated into xmldocs.make because, in
#       general, there will be other documents install besides XML documents
#       and the makefiles for these formats should also include this file.

omf_dest_dir=$(datadir)/omf/@PACKAGE@

omf: omf_timestamp

omf_timestamp: $(omffile)
	-for file in $(omffile); do \
	  $(top_builddir)/cl/src/scrollkeeper-preinstall $(docdir)/$(docname).xml $(srcdir)/$$file $(srcdir)/$$file.out; \
	done
	touch omf_timestamp

install-data-hook-omf:
	# Install the OMF files
	$(mkinstalldirs) $(DESTDIR)$(omf_dest_dir)
	for file in $(omffile); do \
		$(INSTALL_DATA) $(srcdir)/$$file.out $(DESTDIR)$(omf_dest_dir)/$$file; \
	done

uninstall-local-omf:
	# Remove our OMF files and directory
	-for file in $(srcdir)/*.omf; do \
		basefile=`basename $$file`; \
		rm -f $(omf_dest_dir)/$$basefile; \
	done
	-rmdir $(omf_dest_dir)
