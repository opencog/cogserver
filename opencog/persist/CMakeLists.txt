# This follows the same directory hierarchy as the atomspace git repo.
ADD_SUBDIRECTORY (cog-storage)

ADD_GUILE_MODULE (FILES
   scm/persist-cog.scm
   MODULE_DESTINATION "${GUILE_SITE_DIR}/opencog"
)
