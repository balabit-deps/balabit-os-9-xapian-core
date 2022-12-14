/** @file
 *  @brief SubMatch class for a remote database.
 */
/* Copyright (C) 2006,2007,2009,2010,2011,2014,2015 Olly Betts
 * Copyright (C) 2007,2008 Lemur Consulting Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#include <config.h>

#include "remotesubmatch.h"

#include "debuglog.h"
#include "msetpostlist.h"
#include "backends/remote/remote-database.h"
#include "weight/weightinternal.h"

RemoteSubMatch::RemoteSubMatch(RemoteDatabase *db_,
			       bool decreasing_relevance_,
			       const vector<Xapian::Internal::opt_intrusive_ptr<Xapian::MatchSpy>> & matchspies_)
	: db(db_),
	  decreasing_relevance(decreasing_relevance_),
	  matchspies(matchspies_)
{
    LOGCALL_CTOR(MATCH, "RemoteSubMatch", db_ | decreasing_relevance_ | matchspies_);
}

bool
RemoteSubMatch::prepare_match(bool nowait,
			      Xapian::Weight::Internal & total_stats)
{
    LOGCALL(MATCH, bool, "RemoteSubMatch::prepare_match", nowait | total_stats);
    Xapian::Weight::Internal remote_stats;
    if (!db->get_remote_stats(nowait, remote_stats)) RETURN(false);
    total_stats += remote_stats;
    RETURN(true);
}

void
RemoteSubMatch::start_match(Xapian::doccount first,
			    Xapian::doccount maxitems,
			    Xapian::doccount check_at_least,
			    Xapian::Weight::Internal & total_stats)
{
    LOGCALL_VOID(MATCH, "RemoteSubMatch::start_match", first | maxitems | check_at_least | total_stats);
    db->send_global_stats(first, maxitems, check_at_least, total_stats);
}

PostList *
RemoteSubMatch::get_postlist(MultiMatch * matcher,
			     Xapian::termcount* total_subqs_ptr,
			     Xapian::Weight::Internal& total_stats)
{
    LOGCALL(MATCH, PostList*, "RemoteSubMatch::get_postlist", matcher | total_subqs_ptr | total_stats);
    (void)matcher;
    Xapian::MSet mset;
    db->get_mset(mset, matchspies);
    percent_factor = mset.internal->percent_factor;
    uncollapsed_upper_bound = mset.internal->uncollapsed_upper_bound;
    total_stats.merge(*(mset.internal->stats));
    // For remote databases we report percent_factor rather than counting the
    // number of subqueries.
    (void)total_subqs_ptr;
    RETURN(new MSetPostList(mset, decreasing_relevance));
}
