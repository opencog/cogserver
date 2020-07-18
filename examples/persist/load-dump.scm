;
; load-dump.scm
;
; Demo showing how to load, or dump, large segments of the AtomSpace,
; including the ENTIRE AtomSpace. Caution: for large AtomSpaces, laoding
; everything is painfully slow, and probably not needed. Thus, one can
; load portions:
;
; load-referers ATOM -- to load only those graphs containing ATOM
; load-atoms-of-type TYPE -- to load only atoms of type TYPE
; load-atomspace -- load everything.
;
; store-referers ATOM -- store all graphs that contain ATOM
; store-atomspace -- store everything.
;
(use-modules (opencog) (opencog persist))
(use-modules (opencog persist-cog))

(cogserver-open "cog://localhost")

; Start by assuming the remote server has some content. If not, then
; create some. If unsure how, re-read the `fetch-store.scm` demo.
;
; Lets get only those atoms that make use of `(Concept "a")`
(load-referers (Concept "a"))

; Print the atomspace contents, and look what we got.
(cog-get-all-roots)

; Now get all PredicateNodes. With luck, there should be at least one,
; the key used to store TruthValues.
(load-atoms-of-type 'Predicate)
(cog-get-all-roots)

; What the heck -- get everything.
(load-atomspace)
(cog-get-all-roots)

; -------------------------------------------------
; Lets create some more atoms, and the store everything.
(Concept "foo" (stv 0.1 0.2))
(Concept "bar" (stv 0.3 0.4))
(Set (List (Set (List (Concept "bazzzz" (stv 0.5 0.6))))))
(store-atomspace)
