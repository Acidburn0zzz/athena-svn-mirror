;; wm-spec.jl -- implement the new (GNOME/KDE) wm hints spec

;; $Id: wm-spec.jl,v 1.1.1.1 2000-11-12 06:28:34 ghudson Exp $

;; Copyright (C) 1999, 2000 John Harper <john@dcs.warwick.ac.uk>

;; This file is part of sawmill.

;; sawmill is free software; you can redistribute it and/or modify it
;; under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.

;; sawmill is distributed in the hope that it will be useful, but
;; WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with sawmill; see the file COPYING.  If not, write to
;; the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

(define-structure sawfish.wm.state.wm-spec

    (export define-wm-spec-window-type
	    define-wm-spec-window-state)

    (open rep
	  rep.system
	  sawfish.wm.misc
	  sawfish.wm.windows
	  sawfish.wm.workspace
	  sawfish.wm.viewport
	  sawfish.wm.state.maximize)

  ;; todo:

  ;; - _NET_WORKAREA
  ;; - _NET_VIRTUAL_ROOTS
  ;; - _NET_PROPERTIES		-- ugh!
  ;; - _NET_WM_NAME		-- needs to be in C code?
  ;; - _NET_WM_STRUT
  ;; - _NET_WM_ICON_GEOMETRY
  ;; - _NET_WM_ICON


;;; constants

  (defconst _NET_WM_MOVERESIZE_SIZE_TOPLEFT 0)
  (defconst _NET_WM_MOVERESIZE_SIZE_TOP 1)
  (defconst _NET_WM_MOVERESIZE_SIZE_TOPRIGHT 2)
  (defconst _NET_WM_MOVERESIZE_SIZE_RIGHT 3)
  (defconst _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT 4)
  (defconst _NET_WM_MOVERESIZE_SIZE_BOTTOM 5)
  (defconst _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT 6)
  (defconst _NET_WM_MOVERESIZE_SIZE_LEFT 7)
  (defconst _NET_WM_MOVERESIZE_MOVE 8)

  (defconst _NET_WM_STATE_REMOVE 0)
  (defconst _NET_WM_STATE_ADD 1)
  (defconst _NET_WM_STATE_TOGGLE 2)

  (define wm-spec-window-id nil)

  (define wm-spec-supported-protocols [])	;XXX set me
  
  (define wm-spec-desktop-layer -1)
  (define wm-spec-dock-layer +1)


;;; setting the client list hints

  (define (wm-spec-update-client-list-hints)
    (define (set-prop lst prop)
      (let loop ((rest lst)
		 (collected '()))
	(cond ((null rest)
	       (set-x-property 'root prop
			       (apply vector (nreverse collected))
			       'WINDOW 32))
	      ((window-mapped-p (car rest))
	       (loop (cdr rest) (cons (window-id (car rest)) collected)))
	      (t (loop (cdr rest) collected)))))
    (set-prop (managed-windows) '_NET_CLIENT_LIST)
    (set-prop (stacking-order) '_NET_CLIENT_LIST_STACKING))


;; setting the desktop / viewport hints

  (define last-workspace nil)
  (define last-workspace-count 0)
  (define last-workspace-names nil)
  (define last-area nil)
  (define last-area-count nil)

  (define (wm-spec-update-workspace-hints)
    (let* ((limits (workspace-limits))
	   (port (screen-viewport))
	   (port-size viewport-dimensions)
	   (total-workspaces (1+ (- (cdr limits) (car limits)))))

      (define (set-ws-hints)
	;; _NET_NUMBER_OF_DESKTOPS
	(unless (equal last-workspace-count total-workspaces)
	  (setq last-workspace-count total-workspaces)
	  (set-x-property 'root '_NET_NUMBER_OF_DESKTOPS
			  (vector total-workspaces) 'CARDINAL 32))

	;; _NET_DESKTOP_NAMES
	(unless (equal last-workspace-names workspace-names)
	  (setq last-workspace-names workspace-names)
	  (set-x-text-property 'root '_NET_DESKTOP_NAMES
			       (apply vector workspace-names)))

	;; _NET_CURRENT_DESKTOP
	(unless (equal last-workspace
		       (- current-workspace (car limits)))
	  (setq last-workspace (- current-workspace (car limits)))
	  (set-x-property 'root '_NET_CURRENT_DESKTOP
			  (vector last-workspace) 'CARDINAL 32))

	;; _NET_DESKTOP_GEOMETRY
	(unless (equal last-area-count port-size)
	  (setq last-area-count port-size)
	  (set-x-property 'root '_NET_DESKTOP_GEOMETRY
			  (vector (* (car port-size) (screen-width))
				  (* (cdr port-size) (screen-height)))
			  'CARDINAL 32))

	;; _NET_DESKTOP_VIEWPORT
	(unless (equal last-area port)
	  (let ((view (make-vector (* total-workspaces 2))))
	    (let loop ((i 0))
	      (if (= i total-workspaces)
		  (set-x-property 'root '_NET_DESKTOP_VIEWPORT
				  view 'CARDINAL 32)
		(aset view (* i 2) (* (car port) (screen-width)))
		(aset view (1+ (* i 2)) (* (cdr port) (screen-width)))
		(loop (1+ i)))))))

      (define (set-window-hints w)
	(let
	    ;; XXX the gnome-wm standard sucks..!
	    ((space (and (not (window-get w 'sticky))
			 (window-get w 'swapped-in))))
	  (if space
	      (set-x-property w '_NET_WM_DESKTOP
			      (vector (- space (car limits))) 'CARDINAL 32)
	    (delete-x-property w '_NET_WM_DESKTOP))))
		 
      ;; apparently some pagers don't like it if we place windows
      ;; on (temporarily) non-existent workspaces
      (when (< last-workspace-count total-workspaces)
	(set-ws-hints))

      (map-windows set-window-hints)

      (when (>= last-workspace-count total-workspaces)
	(set-ws-hints))))


;;; setting the focus hints

  (define last-focus nil)

  (define (wm-spec-update-focus-state)
    (let ((focus (input-focus)))
      (unless (eq last-focus focus)
	(setq last-focus focus)
	(set-x-property 'root '_NET_ACTIVE_WINDOW
			(vector (if focus (window-id focus) 0)) 'WINDOW 32))))


;;; setting the window state hints

  (define (wm-spec-update-client-state w)
    (let (state)
      (when (window-get w 'sticky-viewport)
	(setq state (cons '_NET_WM_STATE_STICKY state)))
      (when (window-get w 'shaded)
	(setq state (cons '_NET_WM_STATE_SHADED state)))
      (when (window-maximized-vertically-p w)
	(setq state (cons '_NET_WM_STATE_MAXIMIZED_VERT state)))
      (when (window-maximized-horizontally-p w)
	(setq state (cons '_NET_WM_STATE_MAXIMIZED_HORIZ state)))
      (set-x-property w '_NET_WM_STATE (apply vector state) 'ATOM 32)))


;;; honouring the initially set window state hints

  (define (wm-spec-honour-client-state w)
    ;; XXX is this thing still required
    (let ((class (get-x-text-property w 'WM_CLASS)))
      (when (and class (>= (length class) 2))
	(cond ((and (string= (aref class 1) "Panel")
		    (string= (aref class 0) "panel"))
	       ;; XXX I don't think the GNOME hints specify these things
	       (window-put w 'focus-click-through t)
	       (window-put w 'avoid t))
	      ((string= (aref class 1) "gmc-desktop-icon")
	       (window-put w 'never-focus t)))))

    (let ((space (get-x-property w '_NET_WM_DESKTOP)))
      (when space
	(setq space (aref (nth 2 space) 0))
	(cond ((equal space #xffffffff)
	       (window-put w 'sticky t))
	      ((and (integerp space) (null (window-workspaces w)))
	       (set-window-workspaces w (list space))))))

    (let ((type (get-x-property w '_NET_WM_WINDOW_TYPE)))
      (when type
	(setq type (nth 2 type))
	;; _NET_WM_WINDOW_TYPE is a vector of atoms, the first atom
	;; about which we know something is the type we'll use
	(let loop ((i 0))
	  (cond ((= i (length type)))
		((get (aref type i) 'wm-spec-type)
		 ((get (aref type i) 'wm-spec-type) w))
		(t (loop (1+ i)))))))

    (let ((state (get-x-property w '_NET_WM_WINDOW_STATE)))
      (when state
	(setq state (nth 2 state))
	(let loop ((i 0))
	  (when (< i (length state))
	    (wm-spec-call-state-fun w (aref state i) 'init)
	    (loop (1+ i)))))))


;;; helper functions

  (define (define-wm-spec-window-type x fun) (put x 'wm-spec-type fun))
  (define (define-wm-spec-window-state x fun) (put x 'wm-spec-type fun))

  (define (wm-spec-call-state-fun w state mode)
    (let ((fun (get state 'wm-spec-state)))
      (when fun
	(fun w mode))))

  (define-wm-spec-window-type
   '_NET_WM_WINDOW_TYPE_DESKTOP
   (lambda (w)
     (require 'sawfish.wm.stacking)
     (set-window-depth w wm-spec-desktop-layer)
     (window-put w 'fixed-position t)
     (window-put w 'desktop t)
     (window-put w 'keymap root-window-keymap)))

  (define-wm-spec-window-type
   '_NET_WM_WINDOW_TYPE_DOCK
   (lambda (w)
     (require 'sawfish.wm.stacking)
     (set-window-depth w wm-spec-dock-layer)))

  (define-wm-spec-window-type
   '_NET_WM_WINDOW_TYPE_DIALOG
   (lambda (w)
     (require 'sawfish.wm.frames)
     (set-window-type w 'transient)))

  (define-wm-spec-window-state
   '_NET_WM_STATE_STICKY
   (lambda (w mode)
     (require 'sawfish.wm.state.iconify)
     (case mode
       ((init)
	(window-put w 'sticky-viewport t))
       ((remove)
	(window-put w 'sticky-viewport nil))
       ((add)
	(window-put w 'sticky-viewport t))
       ((toggle)
	(window-put w 'sticky-viewport
		    (not (window-get w 'sticky-viewport)))))
     (unless (eq mode 'init)
       (call-window-hook w 'window-state-change-hook (list '(sticky))))))

  (define (wm-spec-maximize-handler direction)
    (lambda (w mode)
      (require 'sawfish.wm.state.maximize)
      (case mode
	((init)
	 (window-put w (if (eq direction 'vertical)
			   'queued-vertical-maximize
			 'queued-horizontal-maximize) t))
	((remove)
	 (unmaximize-window w direction))
	((add)
	 (maximize-window w direction))
	((toggle)
	 (maximize-window-toggle w direction)))))

  (define-wm-spec-window-state '_NET_WM_STATE_MAXIMIZED_VERT
			       (wm-spec-maximize-handler 'vertical))
  (define-wm-spec-window-state '_NET_WM_STATE_MAXIMIZED_HORIZ
			       (wm-spec-maximize-handler 'horizontal))
  (define-wm-spec-window-state '_NET_WM_STATE_MAXIMIZED
			       (wm-spec-maximize-handler nil))

  (define-wm-spec-window-state
   '_NET_WM_STATE_SHADED
   (lambda (w mode)
     (require 'sawfish.wm.state.shading)
     (case mode
       ((init)
	(window-put w 'shaded t))
       ((add)
	(shade-window w))
       ((remove)
	(unshade-window w))
       ((toggle)
	(toggle-window-shaded w)))))


;;; client messages

  (define (wm-spec-client-message-handler w type data)
    (let ((handled t))
      (case type
	((_NET_CLOSE_WINDOW) (delete-window w))

	((_NET_WM_MOVERESIZE)
	 (require 'sawfish.wm.commands.move-resize)
	 (let ((mode (aref data 2)))
	   (if (eq mode _NET_WM_MOVERESIZE_MOVE)
	       (move-window-interactively w)
	     (let ((move-resize-moving-edges
		    (cond ((eq mode _NET_WM_MOVERESIZE_SIZE_TOPLEFT) '(top left))
			  ((eq mode _NET_WM_MOVERESIZE_SIZE_TOP) '(top))
			  ((eq mode _NET_WM_MOVERESIZE_SIZE_TOPRIGHT) '(top right))
			  ((eq mode _NET_WM_MOVERESIZE_SIZE_BOTTOMLEFT) '(bottom left))
			  ((eq mode _NET_WM_MOVERESIZE_SIZE_BOTTOM) '(bottom))
			  ((eq mode _NET_WM_MOVERESIZE_SIZE_BOTTOMRIGHT) '(bottom right))
			  ((eq mode _NET_WM_MOVERESIZE_SIZE_LEFT) '(left))
			  ((eq mode _NET_WM_MOVERESIZE_SIZE_RIGHT) '(right)))))
	       (resize-window-interactively w)))))

	((_NET_INSERT_DESKTOP)
	 (insert-workspace (workspace-id-from-logical (aref data 0))))

	((_NET_DELETE_DESKTOP)
	 (remove-workspace (workspace-id-from-logical (aref data 0))))

	((_NET_DESKTOP_GEOMETRY)
	 ;; XXX this conflicts with user preferences
	 )

	((_NET_DESKTOP_VIEWPORT)
	 (set-viewport (aref data 0) (aref data 1)))

	((_NET_CURRENT_DESKTOP)
	 (select-workspace (workspace-id-from-logical (aref data 0))))

	((_NET_DESKTOP_NAMES)
	 (setq data (aref data 0))
	 (let loop ((i 0)
		    (out '()))
	   (if (= i (length data))
	       (setq workspace-names (nreverse out))
	     (loop (1+ i) (cons (aref data i) out)))))

	((_NET_ACTIVE_WINDOW)
	 (when (window-mapped-p w))
	 ;; XXX select workspace / viewport automatically?
	 (set-input-focus w))

	((_NET_WM_STATE)
	 (let ((mode (cond ((eql (aref data 0) _NET_WM_STATE_REMOVE) 'remove)
			   ((eql (aref data 0) _NET_WM_STATE_ADD) 'add)
			   ((eql (aref data 0) _NET_WM_STATE_TOGGLE) 'toggle)))
	       (atom1 (x-atom-name (aref data 1)))
	       (atom2 (x-atom-name (aref data 2))))
	   (when (or (and (eq atom1 '_NET_WM_STATE_MAXIMIZED_VERT)
			  (eq atom2 '_NET_WM_STATE_MAXIMIZED_HORIZ))
		     (and (eq atom2 '_NET_WM_STATE_MAXIMIZED_VERT)
			  (eq atom1 '_NET_WM_STATE_MAXIMIZED_HORIZ)))
	     (setq atom1 '_NET_WM_STATE_MAXIMIZED)
	     (setq atom2 nil))
	   (when atom1
	     (wm-spec-call-state-fun w atom1 mode))
	   (when atom2
	     (wm-spec-call-state-fun w atom2 mode))))

	(t (setq handled nil)))
      handled))


;;; initialisation

  (define (wm-spec-init)
    (setq wm-spec-window-id (create-window 'root -200 -200 5 5))

    (set-x-property 'root '_NET_SUPPORTING_WM_CHECK
		    (vector wm-spec-window-id) 'WINDOW 32)
    (set-x-property wm-spec-window-id '_NET_SUPPORTING_WM_CHECK
		    (vector wm-spec-window-id) 'WINDOW 32)
    (set-x-property wm-spec-window-id '_NET_WM_NAME "Sawfish" 'STRING 8)

    (set-x-property 'root '_NET_SUPPORTED wm-spec-supported-protocols 'ATOM 32)

    (wm-spec-update-client-list-hints)
    (wm-spec-update-workspace-hints)

    (add-hook 'workspace-state-change-hook wm-spec-update-workspace-hints)
    (add-hook 'viewport-resized-hook wm-spec-update-workspace-hints)
    (add-hook 'viewport-moved-hook wm-spec-update-workspace-hints)

    (add-hook 'add-window-hook wm-spec-update-client-list-hints)
    (add-hook 'destroy-notify-hook wm-spec-update-client-list-hints)
    (add-hook 'map-notify-hook wm-spec-update-client-list-hints)
    (add-hook 'unmap-notify-hook wm-spec-update-client-list-hints)
    (add-hook 'workspace-state-change-hook wm-spec-update-client-list-hints)

    (add-hook 'before-add-window-hook wm-spec-honour-client-state)
    (add-hook 'add-window-hook wm-spec-update-client-state)
    (call-after-state-changed '(sticky shaded maximized stacking)
			      wm-spec-update-client-state)

    (add-hook 'focus-in-hook wm-spec-update-focus-state)
    (add-hook 'focus-out-hook wm-spec-update-focus-state)

    (add-hook 'client-message-hook wm-spec-client-message-handler)
    (add-hook 'before-exit-hook wm-spec-exit)

    (map-windows wm-spec-update-client-state))

  (define (wm-spec-exit)
    (destroy-window wm-spec-window-id)
    (delete-x-property 'root '_NET_SUPPORTING_WM_CHECK)
    (delete-x-property 'root '_NET_PROTOCOLS)
    (delete-x-property 'root '_NET_DESKTOP_GEOMETRY)
    (delete-x-property 'root '_NET_DESKTOP_VIEWPORT))

  (unless (or wm-spec-window-id batch-mode)
    (wm-spec-init)))
