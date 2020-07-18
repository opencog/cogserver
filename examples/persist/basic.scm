;
; basic.scm
; basic demo.
;
(use-modules (opencog) (opencog persist))
(use-modules (opencog persist-cog))

(cogserver-open "cog://localhost")

; On the server, create an atom `(Concept "b" (stv 0.9 0.2))`. That is,
; create the ConceptNode with some non-default SimpleTruthValue on it.
; Then locally, we can fetch it, and verify we got the right TV.
(fetch-atom (Concept "b"))

; Create `(Concept "a")` locally, and set several values on it,
; and theen push the result out to the server.
(cog-set-value! (Concept "a") (Predicate "flo") (FloatValue 1 2 3))
(cog-set-value! (Concept "a") (Predicate "blo") (FloatValue 4 5 6))
(store-atom (Concept "a"))

; There are several ways to verify that the above worked. One way
; is to log onto the server, and verify that the data arrived there:
; so try '(cog-keys->alist (Concept "a"))` on the server.
;
; A second way is to log out of this session, and start a new one,
; and do a `(fetch-atom (Concept "a"))` to verify that the old values
; were fetched.
;
; A third way, below, is to just delete the keys, as follows:
(cog-set-value! (Concept "a") (Predicate "flo") #f)
(cog-set-value! (Concept "a") (Predicate "blo") #f)

; Verify that they are gone:
(cog-keys (Concept "a"))
(cog-keys->alist (Concept "a"))

; Get them back:
(fetch-atom (Concept "a"))

; Verify that they were delivered:
(cog-keys (Concept "a"))
(cog-keys->alist (Concept "a"))
