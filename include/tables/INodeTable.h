/*
 * Copyright (C) 2018 Hops.io
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
/* 
 * File:   INodeTable.h
 * Author: Mahmoud Ismail <maism@kth.se>
 *
 */

#ifndef INODETABLE_H
#define INODETABLE_H

#include "DBTable.h"
#include "UserTable.h"
#include "GroupTable.h"
#include "FsMutationsLogTable.h"
#include "ProjectTable.h"

#define DOC_TYPE_INODE "inode"

struct INodeRow {
  int mParentId;
  string mName;
  int mPartitionId;
  int mId;
  long mSize;
  int mUserId;
  int mGroupId;
  string mUserName;
  string mGroupName;
  
  int mLogicalTime;
  OperationType mOperation;
  bool mIsDir;
  
  bool is_equal(ProjectRow proj){
    return proj.mInodeName == mName && proj.mInodeParentId == mParentId 
            && proj.mInodePartitionId == mPartitionId;
  }
  
  static string to_delete_json(int inodeId) {
    rapidjson::StringBuffer sbOp;
    rapidjson::Writer<rapidjson::StringBuffer> opWriter(sbOp);

    opWriter.StartObject();

    // update could be used and set operation to delete instead of add
    opWriter.String("delete");
    opWriter.StartObject();

    opWriter.String("_id");
    opWriter.Int(inodeId);

    opWriter.EndObject();

    opWriter.EndObject();

    return sbOp.GetString();
  }

  string to_create_json(int datasetId, int projectId) {
    stringstream out;
    rapidjson::StringBuffer sbOp;
    rapidjson::Writer<rapidjson::StringBuffer> opWriter(sbOp);

    opWriter.StartObject();

    opWriter.String("update");
    opWriter.StartObject();

    opWriter.String("_id");
    opWriter.Int(mId);

    opWriter.EndObject();

    opWriter.EndObject();

    out << sbOp.GetString() << endl;

    rapidjson::StringBuffer sbDoc;
    rapidjson::Writer<rapidjson::StringBuffer> docWriter(sbDoc);

    docWriter.StartObject();
    docWriter.String("doc");
    docWriter.StartObject();

    docWriter.String("doc_type");
    docWriter.String(DOC_TYPE_INODE);

    docWriter.String("parent_id");
    docWriter.Int(mParentId);

    docWriter.String("partition_id");
    docWriter.Int(mPartitionId);

    docWriter.String("dataset_id");
    docWriter.Int(datasetId);

    docWriter.String("project_id");
    docWriter.Int(projectId);

    docWriter.String("name");
    docWriter.String(mName.c_str());

    docWriter.String("operation");
    docWriter.Int(mOperation);

    docWriter.String("timestamp");
    docWriter.Int(mLogicalTime);

    docWriter.String("size");
    docWriter.Int64(mSize);

    docWriter.String("user");
    docWriter.String(mUserName.c_str());


    docWriter.String("group");
    docWriter.String(mGroupName.c_str());

    docWriter.EndObject();

    docWriter.String("doc_as_upsert");
    docWriter.Bool(true);

    docWriter.EndObject();

    out << sbDoc.GetString() << endl;
    return out.str();
  }
};

typedef boost::unordered_map<int, INodeRow> INodeMap;
typedef vector<INodeRow> INodeVec;

class INodeTable : public DBTable<INodeRow> {
public:

  INodeTable(int lru_cap) : DBTable("hdfs_inodes"), mUsersTable(lru_cap),
  mGroupsTable(lru_cap) {
    addColumn("parent_id");
    addColumn("name");
    addColumn("partition_id");
    addColumn("id");
    addColumn("size");
    addColumn("user_id");
    addColumn("group_id");
    addColumn("logical_time");
    addColumn("is_dir");
  }

  INodeRow getRow(NdbRecAttr* values[]) {
    INodeRow row;
    row.mParentId = values[0]->int32_value();
    row.mName = get_string(values[1]);
    row.mPartitionId = values[2]->int32_value();
    row.mId = values[3]->int32_value();
    row.mSize = values[4]->int64_value();
    row.mUserId = values[5]->int32_value();
    row.mGroupId = values[6]->int32_value();
    row.mLogicalTime = values[7]->int32_value();
    row.mIsDir = values[8]->int8_value() == 1;
    return row;
  }

  INodeVec getByParentId(Ndb* connection, int parentId){
    AnyMap key;
    key[0] = parentId;
    INodeVec inodes = doRead(connection, "pidex", key);
    INodeVec results;
    for(INodeVec::iterator it = inodes.begin(); it!=inodes.end(); ++it){
      INodeRow row = *it;
      row.mUserName = mUsersTable.get(connection, row.mUserId).mName;
      row.mGroupName = mGroupsTable.get(connection, row.mGroupId).mName;
      results.push_back(row);
    }
    return results;
  }

  INodeRow getByInodeId(Ndb* connection, int inodeId) {
    AnyMap key;
    key[3] = inodeId;
    INodeVec inodes = doRead(connection, "inode_idx", key);
    INodeRow row;
    if (inodes.size() > 1) {
      LOG_ERROR("INodeId must be unique, got " << inodes.size()
              << " rows for InodeId " << inodeId);
      return row;
    }

    if (inodes.size() == 1) {
      row = inodes[0];
      row.mUserName = mUsersTable.get(connection, row.mUserId).mName;
      row.mGroupName = mGroupsTable.get(connection, row.mGroupId).mName;
    }

    return row;
  }
  
  INodeRow get(Ndb* connection, int parentId, string name, int partitionId) {
    AnyMap a;
    a[0] = parentId;
    a[1] = name;
    a[2] = partitionId;
    return DBTable<INodeRow>::doRead(connection, a);
  }

  INodeMap get(Ndb* connection, Fmq* data_batch) {
    AnyVec anyVec;
    boost::unordered_map<int, FsMutationRow> mutationsByInode;
    for (Fmq::iterator it = data_batch->begin(); it != data_batch->end(); ++it) {
      FsMutationRow row = *it;
      if (row.mOperation == Delete) {
        continue;
      }
      mutationsByInode[row.mInodeId] = row;

      AnyMap pk;
      pk[0] = row.mParentId;
      pk[1] = row.mInodeName;
      pk[2] = row.mPartitionId;
      anyVec.push_back(pk);
    }

    INodeVec inodes = doRead(connection, anyVec);

    UISet user_ids, group_ids;
    for (INodeVec::iterator it = inodes.begin(); it != inodes.end(); ++it) {
      INodeRow row = *it;
      user_ids.insert(row.mUserId);
      group_ids.insert(row.mGroupId);
    }

    mUsersTable.updateUsersCache(connection, user_ids);
    mGroupsTable.updateGroupsCache(connection, group_ids);

    INodeMap result;

    for (INodeVec::iterator it = inodes.begin(); it != inodes.end(); ++it) {
      INodeRow row = *it;
      FsMutationRow fsrow = mutationsByInode[row.mId];
      row.mLogicalTime = fsrow.mLogicalTime;
      row.mOperation = fsrow.mOperation;
      row.mUserName = mUsersTable.getFromCache(row.mUserId);
      row.mGroupName = mGroupsTable.getFromCache(row.mGroupId);
      result[row.mId] = row;
    }

    return result;
  }

  INodeRow currRow(Ndb* connection) {
    INodeRow row = DBTable<INodeRow>::currRow();
    row.mUserName = mUsersTable.get(connection, row.mUserId).mName;
    row.mGroupName = mGroupsTable.get(connection, row.mGroupId).mName;
    return row;
  }

private:
  UserTable mUsersTable;
  GroupTable mGroupsTable;

};

#endif /* INODETABLE_H */
