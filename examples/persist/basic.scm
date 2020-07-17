;
; basic.scm
; basic demo.
;
(use-modules (opencog) (opencog persist))
(use-modules (opencog persist-cog))

(cogserver-open "cog://localhost")

(Concept "a")

(fetch-atom (Concept "b"))
