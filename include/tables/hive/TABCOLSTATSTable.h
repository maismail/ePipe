/*
 * This file is part of ePipe
 * Copyright (C) 2020, Logical Clocks AB. All rights reserved
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef EPIPE_TABCOLSTATSTABLE_H
#define EPIPE_TABCOLSTATSTABLE_H

#include "tables/DBTable.h"

struct TABCOLSTATSRow {
  Int64 mTBLID;
};

class TABCOLSTATSTable : public DBTable<TABCOLSTATSRow> {

public:
  TABCOLSTATSTable() : DBTable("TAB_COL_STATS") {
    addColumn("TBL_ID");
  }

  TABCOLSTATSRow getRow(NdbRecAttr *value[]) {
    TABCOLSTATSRow row;
    row.mTBLID = value[0]->int64_value();
    return row;
  }

  void remove(Ndb *conn, Int64 TBLID)
  {
    AnyMap key;
    key[0] = TBLID;
    int count = deleteByIndex(conn, "TCSTIDIndex", key);
    LOG_INFO("Removed " << count << " entries of TAB_COL_STATS for TBL_ID: " << TBLID);
  }
};

#endif //EPIPE_TABCOLSTATSTABLE_H
