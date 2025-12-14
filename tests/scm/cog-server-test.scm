#! /usr/bin/env -S guile -s
!#
;
; cog-server-test.scm -- Test that the cogserver module loads and works
;
(use-modules (opencog))
(use-modules (opencog test-runner))
(use-modules (opencog cogserver))

(opencog-test-runner)
(define tname "cog-server-test")
(test-begin tname)

(test-assert "CogServerNode-type-exists"
   (defined? 'CogServerNode))

(test-assert "CogServerNode-create"
   (let ((csn (CogServerNode "test-server")))
      (cog-atom? csn)))

;; Helper function to test if a port can be connected to
(define (can-connect-to-port? port)
   (catch #t
      (lambda ()
         (let ((sock (socket AF_INET SOCK_STREAM 0)))
            (connect sock AF_INET INADDR_LOOPBACK port)
            (close-port sock)
            #t))
      (lambda (key . args)
         #f)))

;; Helper to start a cogserver with given name and ports
(define (start-named-cogserver name telnet-port web-port)
   (define csn (CogServerNode name))
   (cog-set-value! csn (Predicate "*-telnet-port-*") (FloatValue telnet-port))
   (cog-set-value! csn (Predicate "*-web-port-*") (FloatValue web-port))
   (cog-set-value! csn (Predicate "*-mcp-port-*") (FloatValue 0))
   (cog-set-value! csn (Predicate "*-start-*") (VoidValue))
   csn)

;; Helper to stop a cogserver
(define (stop-named-cogserver csn)
   (cog-set-value! csn (Predicate "*-stop-*") (VoidValue)))

;; ===== Test 1: Single cogserver =====
(define single-telnet-port 17101)
(define single-web-port 18181)
(define single-csn #f)

(test-assert "single-start"
   (begin
      (set! single-csn (start-named-cogserver "cogserver-single" single-telnet-port single-web-port))
      (cog-atom? single-csn)))

(test-assert "single-telnet-reachable"
   (can-connect-to-port? single-telnet-port))

(test-assert "single-web-reachable"
   (can-connect-to-port? single-web-port))

(test-assert "single-stop"
   (begin
      (stop-named-cogserver single-csn)
      #t))

(test-assert "single-telnet-closed"
   (not (can-connect-to-port? single-telnet-port)))

(test-assert "single-web-closed"
   (not (can-connect-to-port? single-web-port)))

;; ===== Test 2: Three cogservers simultaneously =====
(define s1-telnet 17201) (define s1-web 18281)
(define s2-telnet 17202) (define s2-web 18282)
(define s3-telnet 17203) (define s3-web 18283)
(define csn1 #f)
(define csn2 #f)
(define csn3 #f)

(test-assert "three-start-A"
   (begin
      (set! csn1 (start-named-cogserver "cogserver-A" s1-telnet s1-web))
      (cog-atom? csn1)))

(test-assert "three-start-B"
   (begin
      (set! csn2 (start-named-cogserver "cogserver-B" s2-telnet s2-web))
      (cog-atom? csn2)))

(test-assert "three-start-C"
   (begin
      (set! csn3 (start-named-cogserver "cogserver-C" s3-telnet s3-web))
      (cog-atom? csn3)))

;; Test connectivity to all six ports
(test-assert "three-A-telnet-reachable"
   (can-connect-to-port? s1-telnet))

(test-assert "three-A-web-reachable"
   (can-connect-to-port? s1-web))

(test-assert "three-B-telnet-reachable"
   (can-connect-to-port? s2-telnet))

(test-assert "three-B-web-reachable"
   (can-connect-to-port? s2-web))

(test-assert "three-C-telnet-reachable"
   (can-connect-to-port? s3-telnet))

(test-assert "three-C-web-reachable"
   (can-connect-to-port? s3-web))

;; Stop all cogservers
(test-assert "three-stop-all"
   (begin
      (stop-named-cogserver csn1)
      (stop-named-cogserver csn2)
      (stop-named-cogserver csn3)
      #t))

;; Verify all ports are closed
(test-assert "three-A-telnet-closed"
   (not (can-connect-to-port? s1-telnet)))

(test-assert "three-A-web-closed"
   (not (can-connect-to-port? s1-web)))

(test-assert "three-B-telnet-closed"
   (not (can-connect-to-port? s2-telnet)))

(test-assert "three-B-web-closed"
   (not (can-connect-to-port? s2-web)))

(test-assert "three-C-telnet-closed"
   (not (can-connect-to-port? s3-telnet)))

(test-assert "three-C-web-closed"
   (not (can-connect-to-port? s3-web)))

(test-end tname)

(opencog-test-end)
