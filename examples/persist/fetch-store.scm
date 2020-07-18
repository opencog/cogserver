;
; fetch-store.scm
;
; Demo of basic fetching individual atoms from the remote server,
; and storing (sending) them to the server for safe-keeping.
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
;
; -----------------------------------------------------------
; One can get just the incoming set in a similar fashion.
; Assume that, for example, the remote server contains
;    (List (Concept "a")(Concept "b"))
; Then `(fetch-incoming-set (Concept "a"))` will get the ListLink.
; Similarly, `(fetch-incoming-by-type (Concept "a") 'List)` will
; get only the ListLink, and not other types.
;
; Start by creating the ListLink, and storing it. And also a SetLink.
(store-atom (List (Concept "a")(Concept "b")))
(store-atom (Set (Concept "a")(Concept "b")))

; Erase both of them locally. This also erases `(Concept "a")` in the
; local atomspace.
(cog-extract-recursive! (Concept "a"))

; Recreate `(Concept "a")` but verify that nothing contains it:
(cog-incoming-set (Concept "a"))

; Fetch only the ListLink
(fetch-incoming-by-type (Concept "a") 'List)
(cog-incoming-set (Concept "a"))

; Fetch everything
(fetch-incoming-set (Concept "a"))
(cog-incoming-set (Concept "a"))
