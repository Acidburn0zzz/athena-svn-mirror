/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2001
 *	Sleepycat Software.  All rights reserved.
 */
/*
 * Copyright (c) 1995, 1996
 *	The President and Fellows of Harvard University.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Margo Seltzer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "db_config.h"

#ifndef lint
static const char revid[] = "Id: txn.c,v 11.125 2001/11/16 16:28:17 bostic Exp ";
#endif /* not lint */

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>

#if TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#include <string.h>
#endif

#include "db_int.h"
#include "db_page.h"
#include "db_shash.h"
#include "hash.h"
#include "lock.h"
#include "log.h"
#include "txn.h"

/*
 * __txn_isvalid enumerated types.  We cannot simply use the transaction
 * statuses, because different statuses need to be handled differently
 * depending on the caller.
 */
typedef enum {
	TXN_OP_ABORT,
	TXN_OP_COMMIT,
	TXN_OP_DISCARD,
	TXN_OP_PREPARE
} txnop_t;

static int  __txn_begin_int __P((DB_TXN *, int));
static void __txn_end __P((DB_TXN *, int));
static int  __txn_isvalid __P((const DB_TXN *, TXN_DETAIL **, txnop_t));
static int  __txn_set_timeout __P(( DB_TXN *, db_timeout_t, u_int32_t));
static int  __txn_undo __P((DB_TXN *));

#ifndef db_create
/*
 * txn_abort --
 * txn_begin --
 * txn_commit --
 *
 * When we switched to methods in 4.0, we guessed txn_{abort,begin,commit}
 * were the interfaces applications would likely use and not be willing to
 * change, due to the sheer volume of the calls.  Provide wrappers -- we
 * could do txn_abort and txn_commit using macros, but not txn_begin, as
 * the name of the field is txn_begin, we didn't want to modify it.
 *
 * The issue with txn_begin hits us in another way.  If configured with the
 * --with-uniquename option, we use #defines to re-define DB's interfaces
 * to unique names.  We can't do that for these functions because txn_begin
 * is also a field name in the DB_ENV structure, and the #defines we use go
 * at the end of the db.h file -- we get control too late to #define a field
 * name.  So, modify the script that generates the unique names #defines to
 * not generate them for these three functions, and don't include the three
 * functions in libraries built with that configuration option.
 *
 * EXTERN: int txn_abort __P((DB_TXN *));
 * EXTERN: int txn_begin __P((DB_ENV *, DB_TXN *, DB_TXN **, u_int32_t));
 * EXTERN: int txn_commit __P((DB_TXN *, u_int32_t));
 */
int
txn_abort(txnp)
	DB_TXN *txnp;
{
	return (txnp->abort(txnp));
}

int
txn_begin(dbenv, parent, txnpp, flags)
	DB_ENV *dbenv;
	DB_TXN *parent, **txnpp;
	u_int32_t flags;
{
	return (dbenv->txn_begin(dbenv, parent, txnpp, flags));
}

int
txn_commit(txnp, flags)
	DB_TXN *txnp;
	u_int32_t flags;
{
	return (txnp->commit(txnp, flags));
}
#endif /* !db_create */

/*
 * __txn_begin --
 *	This is a wrapper to the actual begin process.  Normal transaction
 * begin allocates a DB_TXN structure for the caller, while XA transaction
 * begin does not.  Other than that, both call into common __txn_begin_int
 * code.
 *
 * Internally, we use TXN_DETAIL structures, but the DB_TXN structure
 * provides access to the transaction ID and the offset in the transaction
 * region of the TXN_DETAIL structure.
 *
 * PUBLIC: int __txn_begin __P((DB_ENV *, DB_TXN *, DB_TXN **, u_int32_t));
 */
int
__txn_begin(dbenv, parent, txnpp, flags)
	DB_ENV *dbenv;
	DB_TXN *parent, **txnpp;
	u_int32_t flags;
{
	DB_LOCKREGION *region;
	DB_TXN *txn;
	int ret;

	*txnpp = NULL;
	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG(dbenv, dbenv->tx_handle, "txn_begin", DB_INIT_TXN);

	if ((ret = __db_fchk(dbenv,
	    "txn_begin", flags,
	    DB_DIRTY_READ | DB_TXN_NOWAIT |
	    DB_TXN_NOSYNC | DB_TXN_SYNC)) != 0)
		return (ret);
	if ((ret = __db_fcchk(dbenv,
	    "txn_begin", flags, DB_TXN_NOSYNC, DB_TXN_SYNC)) != 0)
		return (ret);

	if ((ret = __os_calloc(dbenv, 1, sizeof(DB_TXN), &txn)) != 0)
		return (ret);

	txn->mgrp = dbenv->tx_handle;
	txn->parent = parent;
	TAILQ_INIT(&txn->kids);
	txn->flags = TXN_MALLOC;
	if (LF_ISSET(DB_DIRTY_READ))
		F_SET(txn, TXN_DIRTY_READ);
	if (LF_ISSET(DB_TXN_NOSYNC))
		F_SET(txn, TXN_NOSYNC);
	if (LF_ISSET(DB_TXN_SYNC))
		F_SET(txn, TXN_SYNC);
	if (LF_ISSET(DB_TXN_NOWAIT))
		F_SET(txn, TXN_NOWAIT);

	if ((ret = __txn_begin_int(txn, 0)) != 0)
		goto err;

	if (parent != NULL)
		TAILQ_INSERT_HEAD(&parent->kids, txn, klinks);

	if (LOCKING_ON(dbenv)) {
		region = ((DB_LOCKTAB *)dbenv->lk_handle)->reginfo.primary;
		if (parent != NULL) {
			ret = __lock_inherit_timeout(dbenv,
			    parent->txnid, txn->txnid);
			/* No parent locker set yet. */
			if (ret == EINVAL) {
				parent = NULL;
				ret = 0;
			}
			if (ret != 0)
				goto err;
		}

		/*
		 * Parent is NULL if we have no parent
		 * or it has no timeouts set.
		 */
		if (parent == NULL && region->tx_timeout != 0)
			if ((ret = __lock_set_timeout(dbenv, txn->txnid,
			    region->tx_timeout, DB_SET_TXN_TIMEOUT)) != 0)
				goto err;
	}

	*txnpp = txn;
	return (0);

err:
	__os_free(dbenv, txn, sizeof(DB_TXN));
	return (ret);
}

/*
 * __txn_xa_begin --
 *	XA version of txn_begin.
 *
 * PUBLIC: int __txn_xa_begin __P((DB_ENV *, DB_TXN *));
 */
int
__txn_xa_begin(dbenv, txn)
	DB_ENV *dbenv;
	DB_TXN *txn;
{
	PANIC_CHECK(dbenv);

	memset(txn, 0, sizeof(DB_TXN));

	txn->mgrp = dbenv->tx_handle;

	return (__txn_begin_int(txn, 0));
}

/*
 * __txn_compensate_begin
 *	Begin an compensation transaction.  This is a special interface
 * that is used only for transactions that must be started to compensate
 * for actions during an abort.  Currently only used for allocations.
 *
 * PUBLIC: int __txn_compensate_begin __P((DB_ENV *, DB_TXN **txnp));
 */
int
__txn_compensate_begin(dbenv, txnpp)
	DB_ENV *dbenv;
	DB_TXN **txnpp;
{
	DB_TXN *txn;
	int ret;

	PANIC_CHECK(dbenv);

	if ((ret = __os_calloc(dbenv, 1, sizeof(DB_TXN), &txn)) != 0)
		return (ret);

	txn->mgrp = dbenv->tx_handle;

	*txnpp = txn;
	F_SET(txn, TXN_COMPENSATE);
	return (__txn_begin_int(txn, 1));
}

/*
 * __txn_begin_int --
 *	Normal DB version of txn_begin.
 */
static int
__txn_begin_int(txn, internal)
	DB_TXN *txn;
	int internal;
{
	DB_ENV *dbenv;
	DB_LSN begin_lsn;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	TXN_DETAIL *td;
	size_t off;
	u_int32_t id, min_inuse;
	int ret;

	mgr = txn->mgrp;
	dbenv = mgr->dbenv;
	region = mgr->reginfo.primary;

	/*
	 * We do not have to write begin records (and if we do not, then we
	 * need never write records for read-only transactions).  However,
	 * we do need to find the current LSN so that we can store it in the
	 * transaction structure, so we can know where to take checkpoints.
	 */
	if (LOGGING_ON(dbenv) &&
	    (ret = dbenv->log_put(dbenv, &begin_lsn, NULL, DB_CURLSN)) != 0)
		goto err2;

	R_LOCK(dbenv, &mgr->reginfo);
	if (!F_ISSET(txn, TXN_COMPENSATE) && F_ISSET(region, TXN_IN_RECOVERY)) {
		__db_err(dbenv, "operation not permitted during recovery.");
		ret = EINVAL;
		goto err1;
	}

	/* Make sure that we aren't still recovering prepared transactions. */
	if (!internal && region->stat.st_nrestores != 0) {
		__db_err(dbenv,
"txn_begin: recovery of prepared but not yet committed transactions is incomplete.");
		ret = EINVAL;
		goto err1;
	}

	/*
	 * Allocate a new transaction id.  If we wrap around then we
	 * find the minimum currently in use and make sure we
	 * can stay below that.  This code is similar to code
	 * in __lock_id for recovering locker ids.
	 */

	if (region->last_txnid == region->cur_maxid) {
		min_inuse = TXN_INVALID;
		/* If cur_maxid is at the top we are wrapping around. */
		if (region->cur_maxid == TXN_INVALID)
			region->cur_maxid = TXN_MINIMUM;

		/*
		 * Traverse teh active transactions and find the
		 * lowest txnid which is in use.
		 */
		for (td = SH_TAILQ_FIRST(&region->active_txn, __txn_detail);
		    td != NULL;
		    td = SH_TAILQ_NEXT(td, links, __txn_detail)) {
			if (td->txnid > region->cur_maxid
			     && td->txnid < min_inuse)
				min_inuse = td->txnid;
		}
		/* Set the maxid to the new lowest. */
		region->cur_maxid = min_inuse;
		/*
		 * Check to see if the current txnid is wrapping around.
		 * Since this should represent the last id allocated
		 * and we don't use TXN_MINIMUM  things are
		 * a little tricky, we don't want to allocate
		 * the first id if its inuse.
		 */
		if (region->last_txnid == TXN_INVALID) {
			if (min_inuse == TXN_MINIMUM + 1)
				region->last_txnid = min_inuse;
			else
				region->last_txnid = TXN_MINIMUM;
		}

		/* See if we fixed the situation. */
		if (region->last_txnid == region->cur_maxid) {
			__db_err(dbenv,
	"txn_begin: transaction ID wrapped.  Exit the database environment\nand restart the application as if application failure had occurred");
			ret = EINVAL;
			goto err1;
		}
	}

	/* Allocate a new transaction detail structure. */
	if ((ret =
	    __db_shalloc(mgr->reginfo.addr, sizeof(TXN_DETAIL), 0, &td)) != 0) {
		__db_err(dbenv,
		     "Unable to allocate memory for transaction detail");
		goto err1;
	}

	/* Place transaction on active transaction list. */
	SH_TAILQ_INSERT_HEAD(&region->active_txn, td, links, __txn_detail);

	id = ++region->last_txnid;
	++region->stat.st_nbegins;
	if (++region->stat.st_nactive > region->stat.st_maxnactive)
		region->stat.st_maxnactive = region->stat.st_nactive;

	td->txnid = id;
	td->begin_lsn = begin_lsn;
	ZERO_LSN(td->last_lsn);
	td->status = TXN_RUNNING;
	if (txn->parent != NULL)
		td->parent = txn->parent->off;
	else
		td->parent = INVALID_ROFF;

	td->flags = 0;
	off = R_OFFSET(&mgr->reginfo, td);
	R_UNLOCK(dbenv, &mgr->reginfo);

	ZERO_LSN(txn->last_lsn);
	txn->txnid = id;
	txn->off = off;

	txn->abort = __txn_abort;
	txn->commit = __txn_commit;
	txn->discard = __txn_discard;
	txn->id = __txn_id;
	txn->prepare = __txn_prepare;
	txn->set_timeout = __txn_set_timeout;

	/*
	 * If this is a transaction family, we must link the child to the
	 * maximal grandparent in the lock table for deadlock detection.
	 */
	if (txn->parent != NULL && LOCKING_ON(dbenv))
		if ((ret = __lock_addfamilylocker(dbenv,
		    txn->parent->txnid, txn->txnid)) != 0)
			goto err2;

	if (F_ISSET(txn, TXN_MALLOC)) {
		MUTEX_THREAD_LOCK(dbenv, mgr->mutexp);
		TAILQ_INSERT_TAIL(&mgr->txn_chain, txn, links);
		MUTEX_THREAD_UNLOCK(dbenv, mgr->mutexp);
	}

	return (0);

err1:	R_UNLOCK(dbenv, &mgr->reginfo);

err2:	return (ret);
}

/*
 * __txn_commit --
 *	Commit a transaction.
 *
 * PUBLIC: int __txn_commit __P((DB_TXN *, u_int32_t));
 */
int
__txn_commit(txnp, flags)
	DB_TXN *txnp;
	u_int32_t flags;
{
	DB_ENV *dbenv;
	DB_LOCKREQ request;
	DB_TXN *kid;
	int ret;

	dbenv = txnp->mgrp->dbenv;

	PANIC_CHECK(dbenv);

	if ((ret = __txn_isvalid(txnp, NULL, TXN_OP_COMMIT)) != 0)
		return (ret);

	/*
	 * We clear flags that are incorrect, ignoring any flag errors, and
	 * default to synchronous operations.  By definition, transaction
	 * handles are dead when we return, and this error should never
	 * happen, but we don't want to fail in the field 'cause the app is
	 * specifying the wrong flag for some reason.
	 */
	if (__db_fchk(dbenv,
	    "DB_TXN->commit", flags, DB_TXN_NOSYNC | DB_TXN_SYNC) != 0)
		flags = DB_TXN_SYNC;
	if (__db_fcchk(dbenv,
	    "DB_TXN->commit", flags, DB_TXN_NOSYNC, DB_TXN_SYNC) != 0)
		flags = DB_TXN_SYNC;
	if (LF_ISSET(DB_TXN_NOSYNC)) {
		F_CLR(txnp, TXN_SYNC);
		F_SET(txnp, TXN_NOSYNC);
	}
	if (LF_ISSET(DB_TXN_SYNC)) {
		F_CLR(txnp, TXN_NOSYNC);
		F_SET(txnp, TXN_SYNC);
	}

	/*
	 * Commit any unresolved children.  If there's an error, abort any
	 * unresolved children and the parent.
	 */
	while ((kid = TAILQ_FIRST(&txnp->kids)) != NULL)
		if ((ret = kid->commit(kid, flags)) != 0) {
			while ((kid = TAILQ_FIRST(&txnp->kids)) != NULL)
				(void)kid->abort(kid);
			goto err;
		}

	/*
	 * If there are any log records, write a log record and sync the log,
	 * else do no log writes.  If the commit is for a child transaction,
	 * we do not need to commit the child synchronously since it may still
	 * abort (if its parent aborts), and otherwise its parent or ultimate
	 * ancestor will write synchronously.
	 */
	if (LOGGING_ON(dbenv) && !IS_ZERO_LSN(txnp->last_lsn)) {
		if (txnp->parent == NULL) {
			request.op = DB_LOCK_PUT_READ;
			if (LOCKING_ON(dbenv) && (ret = dbenv->lock_vec(
			   dbenv, txnp->txnid, 0, &request, 1, NULL)) != 0)
				goto err;

			if ((ret = __txn_regop_log(dbenv,
			    txnp, &txnp->last_lsn,
			    DB_COMMIT |
			    ((F_ISSET(dbenv, DB_ENV_TXN_NOSYNC) &&
			    !F_ISSET(txnp, TXN_SYNC)) ||
			    F_ISSET(txnp, TXN_NOSYNC) ?  0 : DB_FLUSH),
			    TXN_COMMIT, (int32_t)time(NULL))) != 0)
				goto err;
		} else {
			/* Log the commit in the parent! */
			if ((ret = __txn_child_log(dbenv,
			    txnp->parent, &txnp->parent->last_lsn,
			    0, txnp->txnid, &txnp->last_lsn)) != 0) {
				goto err;
			}

			F_SET(txnp->parent, TXN_CHILDCOMMIT);
		}
	}

	__txn_end(txnp, 1);
	return (0);

err:	(void)txnp->abort(txnp);
	return (ret);
}

/*
 * __txn_abort --
 *	Abort a transaction.
 *
 * PUBLIC: int __txn_abort __P((DB_TXN *));
 */
int
__txn_abort(txnp)
	DB_TXN *txnp;
{
	DB_ENV *dbenv;
	DB_LOCKREQ request;
	DB_TXN *kid;
	TXN_DETAIL *td;
	int ret, t_ret;

	dbenv = txnp->mgrp->dbenv;

	PANIC_CHECK(dbenv);

	if ((ret = __txn_isvalid(txnp, &td, TXN_OP_ABORT)) != 0)
		return (ret);

	/* Abort any unresolved children. */
	while ((kid = TAILQ_FIRST(&txnp->kids)) != NULL)
		if ((t_ret = kid->abort(kid)) != 0 && ret == 0)
			ret = t_ret;

	if (LOCKING_ON(dbenv)) {
		/* Turn off timeouts. */
		if ((t_ret = __lock_set_timeout(dbenv,
		     txnp->txnid, 0, DB_SET_TXN_TIMEOUT)) != 0 && ret == 0)
			ret = t_ret;
		if ((t_ret = __lock_set_timeout(dbenv,
		     txnp->txnid, 0, DB_SET_LOCK_TIMEOUT)) != 0 && ret == 0)
			ret = t_ret;

		request.op = DB_LOCK_UPGRADE_WRITE;
		if ((ret = dbenv->lock_vec(
		    dbenv, txnp->txnid, 0, &request, 1, NULL)) != 0)
			(void)__db_panic(dbenv, ret);
	}
	if ((t_ret = __txn_undo(txnp)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * Normally, we do not need to log aborts.  However, if we
	 * are a distributed transaction (i.e., we have a prepare),
	 * then we log the abort so we know that this transaction
	 * was actually completed.  Even if the log fails; abort/undo
	 * the transaction.
	 */
	if (LOGGING_ON(dbenv) && td->status == TXN_PREPARED &&
	    (t_ret = __txn_regop_log(dbenv, txnp, &txnp->last_lsn,
	    (F_ISSET(dbenv, DB_ENV_TXN_NOSYNC) &&
	    !F_ISSET(txnp, TXN_SYNC)) ||
	    F_ISSET(txnp, TXN_NOSYNC) ?  0 : DB_FLUSH,
	    TXN_ABORT, (int32_t)time(NULL))) != 0 && ret == 0)
		ret = t_ret;

	__txn_end(txnp, 0);

	return (ret);
}

/*
 * __txn_discard --
 *	Free the per-process resources associated with this txn handle.
 *
 * PUBLIC: int __txn_discard __P((DB_TXN *, u_int32_t flags));
 */
int
__txn_discard(txnp, flags)
	DB_TXN *txnp;
	u_int32_t flags;
{
	DB_ENV *dbenv;
	DB_TXN *freep;
	TXN_DETAIL *td;
	int ret;

	COMPQUIET(flags, 0);

	dbenv = txnp->mgrp->dbenv;
	freep = NULL;

	PANIC_CHECK(dbenv);

	if ((ret = __txn_isvalid(txnp, &td, TXN_OP_DISCARD)) != 0)
		return (ret);

	/* Should be no children. */
	DB_ASSERT(TAILQ_FIRST(&txnp->kids) == NULL);
	DB_ASSERT(F_ISSET(td, TXN_RESTORED));

	/* Free the space. */
	MUTEX_THREAD_LOCK(dbenv, txnp->mgrp->mutexp);
	txnp->mgrp->n_discards++;
	if (F_ISSET(txnp, TXN_MALLOC)) {
		TAILQ_REMOVE(&txnp->mgrp->txn_chain, txnp, links);
		freep = txnp;
	}
	MUTEX_THREAD_UNLOCK(dbenv, txnp->mgrp->mutexp);
	if (freep != NULL)
		__os_free(dbenv, freep, sizeof(*freep));

	return (ret);
}

/*
 * __txn_prepare --
 *	Flush the log so a future commit is guaranteed to succeed.
 *
 * PUBLIC: int __txn_prepare __P((DB_TXN *, u_int8_t *));
 */
int
__txn_prepare(txnp, gid)
	DB_TXN *txnp;
	u_int8_t *gid;
{
	DBT xid;
	DB_ENV *dbenv;
	DB_TXN *kid;
	TXN_DETAIL *td;
	int ret;

	dbenv = txnp->mgrp->dbenv;

	PANIC_CHECK(dbenv);

	if ((ret = __txn_isvalid(txnp, &td, TXN_OP_PREPARE)) != 0)
		return (ret);

	if (txnp->parent != NULL) {
		__db_err(dbenv, "Prepare disallowed on child transactions.");
		return (EINVAL);
	}

	/* Commit any unresolved children. */
	while ((kid = TAILQ_FIRST(&txnp->kids)) != NULL)
		if ((ret = kid->commit(kid, DB_TXN_NOSYNC)) != 0)
			return (ret);

	/*
	 * In XA, the global transaction ID in the txn_detail structure is
	 * already set; in a non-XA environment, we must set it here.  XA
	 * requires that the transaction be either ENDED or SUSPENDED when
	 * prepare is called, so we know that if the xa_status isn't in one
	 * of those states, then we are calling prepare directly and we need
	 * to fill in the td->xid.
	 */
	if (LOGGING_ON(dbenv)) {
		memset(&xid, 0, sizeof(xid));
		if (td->xa_status != TXN_XA_ENDED &&
		    td->xa_status != TXN_XA_SUSPENDED)
			/* Regular prepare; fill in the gid. */
			memcpy(td->xid, gid, sizeof(td->xid));

		xid.size = sizeof(td->xid);
		xid.data = td->xid;

		if ((ret = __txn_xa_regop_log(dbenv, txnp, &txnp->last_lsn,
		    DB_COMMIT | ((F_ISSET(dbenv, DB_ENV_TXN_NOSYNC) &&
		    !F_ISSET(txnp, TXN_SYNC)) ||
		    F_ISSET(txnp, TXN_NOSYNC) ? 0 : DB_FLUSH), TXN_PREPARE,
		    &xid, td->format, td->gtrid, td->bqual,
		    &td->begin_lsn)) != 0) {
			__db_err(dbenv, "DB_TXN->prepare: log_write failed %s",
			    db_strerror(ret));
			return (ret);
		}
	}

	MUTEX_THREAD_LOCK(dbenv, txnp->mgrp->mutexp);
	td->status = TXN_PREPARED;
	MUTEX_THREAD_UNLOCK(dbenv, txnp->mgrp->mutexp);
	return (0);
}

/*
 * __txn_id --
 *	Return the transaction ID.
 *
 * PUBLIC: u_int32_t __txn_id __P((DB_TXN *));
 */
u_int32_t
__txn_id(txnp)
	DB_TXN *txnp;
{
	return (txnp->txnid);
}

/*
 * __txn_set_timeout --
 *	Set timeout values in the txn structure.
 */
static int
__txn_set_timeout(txnp, timeout, op)
	DB_TXN *txnp;
	db_timeout_t timeout;
	u_int32_t op;
{
	if (op != DB_SET_TXN_TIMEOUT &&  op != DB_SET_LOCK_TIMEOUT)
		return (__db_ferr(txnp->mgrp->dbenv, "DB_TXN->set_timeout", 0));

	return (__lock_set_timeout(
	    txnp->mgrp->dbenv, txnp->txnid, timeout, op));
}

/*
 * __txn_isvalid --
 *	Return 0 if the txnp is reasonable, otherwise panic.
 */
static int
__txn_isvalid(txnp, tdp, op)
	const DB_TXN *txnp;
	TXN_DETAIL **tdp;
	txnop_t op;
{
	DB_TXNMGR *mgrp;
	DB_TXNREGION *region;
	TXN_DETAIL *tp;

	mgrp = txnp->mgrp;
	region = mgrp->reginfo.primary;

	/* Check for recovery. */
	if (!F_ISSET(txnp, TXN_COMPENSATE) &&
	    F_ISSET(region, TXN_IN_RECOVERY)) {
		__db_err(mgrp->dbenv,
		    "operation not permitted during recovery.");
		goto err;
	}

	/* Check for live cursors. */
	if (txnp->cursors != 0) {
		__db_err(mgrp->dbenv, "transaction has active cursors");
		goto err;
	}

	/* Check transaction's state. */
	tp = (TXN_DETAIL *)R_ADDR(&mgrp->reginfo, txnp->off);
	if (tdp != NULL)
		*tdp = tp;

	/*
	 * Discard is a special case since we're just tossing the
	 * per-process space; there are a lot of problems with the
	 * transaction that we can tolerate.
	 */
	if (op == TXN_OP_DISCARD) {
		/* Transaction is already been reused. */
		if (txnp->txnid != tp->txnid)
			return (0);

		/*
		 * We have the right transaction, we'd better be
		 * a restored transaction.  However, if it's not,
		 * we won't panic the environment since it means
		 * that someone has a bad user-handle, not that the
		 * transaction is corrupt.
		 */
		if (!F_ISSET(tp, TXN_RESTORED)) {
			__db_err(mgrp->dbenv, "not a restored transaction");
			return (EINVAL);
		}

		/*
		 * No matter what state the transaction is in, we can
		 * blow away this process' handle.
		 */
		return (0);
	}

	switch (tp->status) {
	case TXN_ABORTED:
	case TXN_COMMITTED:
	default:
		__db_err(mgrp->dbenv, "transaction already %s",
		    tp->status == TXN_COMMITTED ? "committed" : "aborted");
		goto err;
	case TXN_PREPARED:
		if (op == TXN_OP_PREPARE) {
			__db_err(mgrp->dbenv, "transaction already prepared");
			/*
			 * Txn_prepare doesn't blow away the user handle, so
			 * in this case, give the user the opportunity to
			 * abort or commit.
			 */
			return (EINVAL);
		}
		break;
	case TXN_RUNNING:
		break;
	}

	return (0);

err:	/*
	 * If there's a serious problem with the transaction, panic.  TXN
	 * handles are dead by definition when we return, and if you use
	 * a cursor you forgot to close, we have no idea what will happen.
	 */
	return (__db_panic(mgrp->dbenv, EINVAL));
}

/*
 * __txn_end --
 *	Internal transaction end routine.
 */
static void
__txn_end(txnp, is_commit)
	DB_TXN *txnp;
	int is_commit;
{
	DB_ENV *dbenv;
	DB_LOCKREQ request;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	TXN_DETAIL *tp;
	int do_closefiles, ret;

	mgr = txnp->mgrp;
	dbenv = mgr->dbenv;
	region = mgr->reginfo.primary;
	do_closefiles = 0;

	/* Release the locks. */
	request.op = txnp->parent == NULL ||
	    is_commit == 0 ? DB_LOCK_PUT_ALL : DB_LOCK_INHERIT;

	/*
	 * __txn_end cannot return an error, we MUST return success/failure
	 * from commit or abort, ignoring any internal errors.  So, we panic
	 * if something goes wrong.  We can't deadlock here because we're not
	 * acquiring any new locks, so DB_LOCK_DEADLOCK is just as fatal as
	 * any other error.
	 */
	if (LOCKING_ON(dbenv) && (ret = dbenv->lock_vec(
	    dbenv, txnp->txnid, DB_LOCK_FREE_LOCKER, &request, 1, NULL)) != 0)
		(void)__db_panic(dbenv, ret);

	/* End the transaction. */
	R_LOCK(dbenv, &mgr->reginfo);

	tp = (TXN_DETAIL *)R_ADDR(&mgr->reginfo, txnp->off);
	SH_TAILQ_REMOVE(&region->active_txn, tp, links, __txn_detail);
	if (F_ISSET(tp, TXN_RESTORED)) {
		region->stat.st_nrestores--;
		do_closefiles = region->stat.st_nrestores == 0;
	}
	__db_shalloc_free(mgr->reginfo.addr, tp);

	if (is_commit)
		region->stat.st_ncommits++;
	else
		region->stat.st_naborts++;
	--region->stat.st_nactive;

	R_UNLOCK(dbenv, &mgr->reginfo);

	/*
	 * The transaction cannot get more locks, remove its locker info,
	 * if any.
	 */
	if (LOCKING_ON(dbenv) && (ret =
	    __lock_freefamilylocker(dbenv->lk_handle, txnp->txnid)) != 0)
		(void)__db_panic(dbenv, ret);
	if (txnp->parent != NULL)
		TAILQ_REMOVE(&txnp->parent->kids, txnp, klinks);

	/* Free the space. */
	if (F_ISSET(txnp, TXN_MALLOC)) {
		MUTEX_THREAD_LOCK(dbenv, mgr->mutexp);
		TAILQ_REMOVE(&mgr->txn_chain, txnp, links);
		MUTEX_THREAD_UNLOCK(dbenv, mgr->mutexp);

		__os_free(dbenv, txnp, sizeof(*txnp));
	}

	if (do_closefiles) {
		F_SET((DB_LOG *)dbenv->lg_handle, DBLOG_RECOVER);
		__log_close_files(dbenv);
		F_CLR((DB_LOG *)dbenv->lg_handle, DBLOG_RECOVER);
		mgr->n_discards = 0;
		(void)dbenv->txn_checkpoint(dbenv, 0, 0, DB_FORCE);
	}
}

/*
 * __txn_undo --
 *	Undo the transaction with id txnid.  Returns 0 on success and
 *	errno on failure.
 */
static int
__txn_undo(txnp)
	DB_TXN *txnp;
{
	DBT rdbt;
	DB_ENV *dbenv;
	DB_LOGC *logc;
	DB_LSN key_lsn;
	DB_TXNMGR *mgr;
	int ret, t_ret, threaded;
	void *txnlist;

	mgr = txnp->mgrp;
	dbenv = mgr->dbenv;
	logc = NULL;
	txnlist = NULL;

	if (!LOGGING_ON(dbenv))
		return (0);

	/*
	 * This is the simplest way to code this, but if the mallocs during
	 * recovery turn out to be a performance issue, we can do the
	 * allocation here and use DB_DBT_USERMEM.
	 */
	memset(&rdbt, 0, sizeof(rdbt));
	threaded = F_ISSET(dbenv, DB_ENV_THREAD) ? 1 : 0;
	if (threaded)
		F_SET(&rdbt, DB_DBT_MALLOC);

	key_lsn = txnp->last_lsn;

	/* Allocate a transaction list for children or aborted page creates. */
	if ((ret = __db_txnlist_init(dbenv, 0, 0, &txnlist)) != 0)
		return (ret);

	if (F_ISSET(txnp, TXN_CHILDCOMMIT) &&
	    (ret = __db_txnlist_lsninit(dbenv, txnlist, &txnp->last_lsn)) != 0)
		return (ret);

	if ((ret = dbenv->log_cursor(dbenv, &logc, 0)) != 0)
		goto err;
	while (ret == 0 && !IS_ZERO_LSN(key_lsn)) {
		/*
		 * The dispatch routine returns the lsn of the record
		 * before the current one in the key_lsn argument.
		 */
		if ((ret = logc->get(logc, &key_lsn, &rdbt, DB_SET)) == 0) {
			ret = __db_dispatch(dbenv, NULL,
			    &rdbt, &key_lsn, DB_TXN_ABORT, txnlist);
			if (threaded && rdbt.data != NULL) {
				__os_free(dbenv, rdbt.data, rdbt.size);
				rdbt.data = NULL;
			}
			if (F_ISSET(txnp, TXN_CHILDCOMMIT))
				(void)__db_txnlist_lsnadd(dbenv,
				    txnlist, &key_lsn, 0);
		}
		if (ret == DB_SURPRISE_KID) {
			if ((ret = __db_txnlist_lsninit(
			    dbenv, txnlist, &key_lsn)) == 0)
				F_SET(txnp, TXN_CHILDCOMMIT);
		} else if (ret != 0) {
			__db_err(txnp->mgrp->dbenv,
		    "DB_TXN->abort: Log undo failed for LSN: %lu %lu: %s",
			    (u_long)key_lsn.file, (u_long)key_lsn.offset,
			    db_strerror(ret));
			goto err;
		}
	}

	if (txnlist != NULL)
		ret = __db_do_the_limbo(dbenv, txnlist);

err:	if (logc != NULL && (t_ret = logc->close(logc, 0)) != 0 && ret == 0)
		ret = t_ret;

	if (txnlist != NULL)
		__db_txnlist_end(dbenv, txnlist);
	return (ret);
}

/*
 * Transaction checkpoint.
 * If either kbytes or minutes is non-zero, then we only take the checkpoint
 * more than "minutes" minutes have passed since the last checkpoint or if
 * more than "kbytes" of log data have been written since the last checkpoint.
 * When taking a checkpoint, find the oldest active transaction and figure out
 * its first LSN.  This is the lowest LSN we can checkpoint, since any record
 * written after since that point may be involved in a transaction and may
 * therefore need to be undone in the case of an abort.
 *
 * PUBLIC: int __txn_checkpoint
 * PUBLIC:     __P((DB_ENV *, u_int32_t, u_int32_t, u_int32_t));
 */
int
__txn_checkpoint(dbenv, kbytes, minutes, flags)
	DB_ENV *dbenv;
	u_int32_t kbytes, minutes, flags;
{
	DB_LOG *dblp;
	DB_LSN ckp_lsn, sync_lsn, last_ckp;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	LOG *lp;
	TXN_DETAIL *txnp;
	time_t last_ckp_time, now;
	u_int32_t bytes, mbytes;
	int interval, ret;

	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG(dbenv,
	    dbenv->tx_handle, "txn_checkpoint", DB_INIT_TXN);

	mgr = dbenv->tx_handle;
	region = mgr->reginfo.primary;
	dblp = dbenv->lg_handle;
	lp = dblp->reginfo.primary;

	/*
	 * Check if we need to checkpoint.
	 */
	ZERO_LSN(ckp_lsn);

	if (LF_ISSET(DB_FORCE))
		goto do_ckp;

	R_LOCK(dbenv, &dblp->reginfo);
	mbytes = lp->stat.st_wc_mbytes;
	/*
	 * We add the current buffer offset so as to count bytes that
	 * have not yet been written, but are sitting in the log buffer.
	 */
	bytes = lp->stat.st_wc_bytes + lp->b_off;
	ckp_lsn = lp->lsn;
	R_UNLOCK(dbenv, &dblp->reginfo);

	/* Don't checkpoint a quiescent database. */
	if (bytes == 0 && mbytes == 0)
		return (0);

	if (kbytes != 0 && mbytes * 1024 + bytes / 1024 >= (u_int32_t)kbytes)
		goto do_ckp;

	if (minutes != 0) {
		(void)time(&now);

		R_LOCK(dbenv, &mgr->reginfo);
		last_ckp_time = region->time_ckp;
		R_UNLOCK(dbenv, &mgr->reginfo);

		if (now - last_ckp_time >= (time_t)(minutes * 60))
			goto do_ckp;
	}

	/*
	 * If we checked time and data and didn't go to checkpoint,
	 * we're done.
	 */
	if (minutes != 0 || kbytes != 0)
		return (0);

do_ckp:
	if (IS_ZERO_LSN(ckp_lsn)) {
		R_LOCK(dbenv, &dblp->reginfo);
		ckp_lsn = lp->lsn;
		R_UNLOCK(dbenv, &dblp->reginfo);
	}

	/*
	 * We have to find an LSN such that all transactions begun
	 * before that LSN are complete.
	 */
	R_LOCK(dbenv, &mgr->reginfo);

	if (IS_ZERO_LSN(region->pending_ckp)) {
		for (txnp =
		    SH_TAILQ_FIRST(&region->active_txn, __txn_detail);
		    txnp != NULL;
		    txnp = SH_TAILQ_NEXT(txnp, links, __txn_detail)) {

			/*
			 * Look through the active transactions for the
			 * lowest begin lsn.
			 */
			if (!IS_ZERO_LSN(txnp->begin_lsn) &&
			    log_compare(&txnp->begin_lsn, &ckp_lsn) < 0)
				ckp_lsn = txnp->begin_lsn;
		}
		region->pending_ckp = ckp_lsn;
	} else
		ckp_lsn = region->pending_ckp;

	R_UNLOCK(dbenv, &mgr->reginfo);

	/*
	 * Try three times to sync the mpool buffers up to the specified LSN,
	 * sleeping 1, 2 and 4 seconds between attempts.
	 */
	if (MPOOL_ON(dbenv))
		for (interval = 1;;) {
			/*
			 * memp_sync may change the lsn you pass it, so don't
			 * pass it the actual ckp_lsn, pass it a local instead.
			 */
			sync_lsn = ckp_lsn;
			if ((ret = dbenv->memp_sync(dbenv, &sync_lsn)) == 0)
				break;

			/*
			 * ret == DB_INCOMPLETE means there are still buffers
			 * to flush, the checkpoint is not complete.
			 */
			if (ret == DB_INCOMPLETE) {
				if (interval > 4)
					return (ret);

				(void)__os_sleep(dbenv, interval, 0);
				interval *= 2;
			} else {
				__db_err(dbenv,
		    "txn_checkpoint: failure when flushing the buffer cache %s",
				    db_strerror(ret));
				return (ret);
			}
		}

	if (LOGGING_ON(dbenv)) {
		R_LOCK(dbenv, &mgr->reginfo);
		last_ckp = region->last_ckp;
		ZERO_LSN(region->pending_ckp);
		R_UNLOCK(dbenv, &mgr->reginfo);

		if ((ret = __txn_ckp_log(dbenv,
		    NULL, &ckp_lsn, DB_CHECKPOINT | DB_FLUSH, &ckp_lsn,
		    &last_ckp, (int32_t)time(NULL))) != 0) {
			__db_err(dbenv,
			    "txn_checkpoint: log failed at LSN [%ld %ld] %s",
			    (long)ckp_lsn.file, (long)ckp_lsn.offset,
			    db_strerror(ret));
			return (ret);
		}

		R_LOCK(dbenv, &mgr->reginfo);

		/*
		 * We want to make sure last_ckp only moves forward;  since
		 * we drop locks above and in log_put, it's possible
		 * for two calls to __txn_ckp_log to finish in a different
		 * order from how they were called.
		 */
		if (log_compare(&region->last_ckp, &ckp_lsn) < 0) {
			region->last_ckp = ckp_lsn;
			(void)time(&region->time_ckp);
		}
		R_UNLOCK(dbenv, &mgr->reginfo);
	}
	return (0);
}

/*
 * __txn_activekids --
 *	Return if this transaction has any active children.
 *
 * PUBLIC: int __txn_activekids __P((DB_ENV *, u_int32_t, DB_TXN *));
 */
int
__txn_activekids(dbenv, rectype, txnp)
	DB_ENV *dbenv;
	u_int32_t rectype;
	DB_TXN *txnp;
{
	/*
	 * On a child commit, we know that there are children (i.e., the
	 * commiting child at the least.  In that case, skip this check.
	 */
	if (rectype == DB_txn_child)
		return (0);

	if (TAILQ_FIRST(&txnp->kids) != NULL) {
		__db_err(dbenv, "Child transaction is active");
		return (EPERM);
	}
	return (0);
}

/*
 * __txn_force_abort --
 *	Force an abort record into the log if the commit record
 *	failed to get to disk.
 *
 * PUBLIC: void __txn_force_abort __P((u_int8_t *));
 */
void
__txn_force_abort(buffer)
	u_int8_t *buffer;
{
	u_int32_t cksum, offset, opcode;
	u_int8_t *bp;

	/*
	 * This routine depends on the layout of HDR and the __txn_regop
	 * record in txn.src.  We are passed the beginning of the commit
	 * record in the log buffer and overwrite the commit with an abort
	 * and recalculate the checksum.
	 *
	 *       rectype		   txn_num		lsn
	 */
	offset = sizeof(u_int32_t) + sizeof(u_int32_t) + sizeof(DB_LSN);
	bp = buffer + sizeof(HDR) + offset;
	opcode = TXN_ABORT;
	memcpy(bp, &opcode, sizeof(opcode));
	cksum = __ham_func4(NULL, buffer + sizeof(HDR),
	    offset + sizeof(u_int32_t) + sizeof(int32_t));
	memcpy(buffer + SSZ(HDR, cksum), &cksum, sizeof(cksum));
}

/*
 * __txn_preclose
 *	Before we can close an environment, we need to check if we
 * were in the midst of taking care of restored transactions.  If
 * so, then we need to close the files that we opened.
 *
 * PUBLIC: void __txn_preclose __P((DB_ENV *));
 */
void
__txn_preclose(dbenv)
	DB_ENV *dbenv;
{
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	int do_closefiles;

	mgr = (DB_TXNMGR *)dbenv->tx_handle;
	region = mgr->reginfo.primary;
	do_closefiles = 0;

	R_LOCK(dbenv, &mgr->reginfo);
	if (region != NULL &&
	    region->stat.st_nrestores
	    <= mgr->n_discards && mgr->n_discards != 0)
		do_closefiles = 1;
	R_UNLOCK(dbenv, &mgr->reginfo);

	if (do_closefiles) {
		/*
		 * Set the DBLOG_RECOVER flag while closing these
		 * files so they do not create additional log records
		 * that will confuse future recoveries.
		 */
		F_SET((DB_LOG *)dbenv->lg_handle, DBLOG_RECOVER);
		__log_close_files(dbenv);
		F_CLR((DB_LOG *)dbenv->lg_handle, DBLOG_RECOVER);
	}
}
