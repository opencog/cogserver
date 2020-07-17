;
; basic.scm
; basic demo.
;
(use-modules (opencog) (opencog persist))
(use-modules (opencog persist-cog))

(cogserver-open "cog://localhost")


(fetch-atom (Concept "b"))

(Concept "a")

(cog-set-value! (Concept "a") (Predicate "flo") (FloatValue 1 2 3))
(cog-set-value! (Concept "a") (Predicate "blo") (FloatValue 4 5 6))

(cog-keys->alist (Concept "a"))
