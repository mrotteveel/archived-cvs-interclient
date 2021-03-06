/*
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */
//-*-C++-*-
#include "IB_Catalog.h"

#include "IB_SQLException.h"
#include "IB_Status.h"

#include "IB_Connection.h"
#include "IB_Statement.h"
#include "IB_ResultSet.h"

#include <stdio.h>
#include <string.h> // strlen, strcat, strstr

#define DEFAULT_VALUE_BUFFER_LEN 1024

const char* const 
IB_Catalog::smallintName__ = "SMALLINT";

const char* const 
IB_Catalog::integerName__ = "INTEGER";

const char* const 
IB_Catalog::floatName__ = "FLOAT";

const char* const 
IB_Catalog::doubleName__ = "DOUBLE PRECISION";

const char* const 
IB_Catalog::numericName__ = "NUMERIC";

const char* const 
IB_Catalog::charName__ = "CHAR";

const char* const 
IB_Catalog::varCharName__ = "VARCHAR";

const char* const 
IB_Catalog::blobName__ = "BLOB";

const char* const 
IB_Catalog::dateName__ = "DATE";

const char* const 
IB_Catalog::arrayName__ = "ARRAY";

// See RDB$FIELDS
IB_Catalog::SQLType
IB_Catalog::getSQLType (const int fieldType, 
			const int fieldSubType, 
			const int fieldScale)
{
  if (fieldScale < 0) {
    switch (fieldType) {
    case 7: 
    case 8: 
    case 27: 
      return NUMERIC_SQLTYPE;
    default:
      break;
    }
  }

  switch (fieldType) {
  case 7: 
    return SMALLINT_SQLTYPE;
  case 8: 
    return INTEGER_SQLTYPE;
  case 27: 
  case 11:
    return DOUBLE_SQLTYPE;
  case 10: 
    return FLOAT_SQLTYPE;
  case 14: 
    return CHAR_SQLTYPE;
  case 37: 
    return VARCHAR_SQLTYPE;
  case 35: 
    return TIMESTAMP_SQLTYPE;
  case 261: 
    if (fieldSubType == 1)
      return LONGVARCHAR_SQLTYPE;
    else
      return LONGVARBINARY_SQLTYPE;
  case 9: 
    return OTHER_SQLTYPE;
  default:
    return NULL_SQLTYPE;      
  }
}

IB_Types::IBType
IB_Catalog::getIBType (const int fieldType, 
		       const int fieldSubType, 
		       const int fieldScale)
{
  if (fieldScale < 0) {
    switch (fieldType) {
    case 7: 
      return IB_Types::NUMERIC_SMALLINT_TYPE;
    case 8: 
      return IB_Types::NUMERIC_INTEGER_TYPE;
    case 27: 
      return IB_Types::NUMERIC_DOUBLE_TYPE;
    default:
      break;
    }
  }

  switch (fieldType) {
  case 7: 
    return IB_Types::SMALLINT_TYPE;
  case 8: 
    return IB_Types::INTEGER_TYPE;
  case 27: 
  case 11:
    return IB_Types::DOUBLE_TYPE;
  case 10: 
    return IB_Types::FLOAT_TYPE;
  case 14: 
    return IB_Types::CHAR_TYPE;
  case 37: 
    return IB_Types::VARCHAR_TYPE;
  case 35: 
    return IB_Types::DATE_TYPE;
  case 261: 
    if (fieldSubType == 1)
      return IB_Types::CLOB_TYPE;
    else
      return IB_Types::BLOB_TYPE;
  case 9: 
    return IB_Types::ARRAY_TYPE;
  default:
    return IB_Types::NULL_TYPE;      
  }
}

int
IB_Catalog::getPrecision (const IB_Types::IBType ibType, 
                          const int fieldLength)
{
  switch (ibType) {
  case IB_Types::CHAR_TYPE:    
  case IB_Types::VARCHAR_TYPE: 
    return fieldLength;
  case IB_Types::DATE_TYPE:    
    return 19;
  case IB_Types::INTEGER_TYPE:  
    return 10;  
  case IB_Types::SMALLINT_TYPE:
    return 5;
  case IB_Types::FLOAT_TYPE:   
    return 7;
  case IB_Types::DOUBLE_TYPE:  
    return 15;
  case IB_Types::ARRAY_TYPE:   
  case IB_Types::CLOB_TYPE: 
  case IB_Types::BLOB_TYPE: 
    return 0;
  default:
    return 0;
  }
}

int
IB_Catalog::getColumnSize (const IB_Types::IBType ibType, 
                           const int fieldLength)
{
  return getPrecision (ibType, fieldLength);
}

IB_Statement*
IB_Catalog::createStatement ()
{
  IB_Statement* statement = new IB_Statement (*status_);
  statement->setConnection (*connection_);
  statement->open ();

  IB_Transaction* transaction = new IB_Transaction (*status_);
  transaction->setConnection (*connection_);
  transaction->open ();

  statement->setTransaction (*transaction);

  return statement;
}

IB_Statement*
IB_Catalog::getProcedures (const IB_STRING procedureNamePattern)
{
  IB_Statement* statement = createStatement ();

  sprintf (queryStringBuffer_,  
	   "select"
	   " RDB$PROCEDURE_NAME,"
	   " RDB$DESCRIPTION,"
	   " RDB$PROCEDURE_OUTPUTS,"
	   " RDB$OWNER_NAME "
	   "from"
	   " RDB$PROCEDURES "
	   "where"
	   " (RDB$PROCEDURE_NAME like '%s' or"
	   "  RDB$PROCEDURE_NAME like '%s %%') "
	   "order by 1",
	   procedureNamePattern,
           procedureNamePattern);

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_PROCEDURES);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getProcedureColumns (const IB_STRING procedureNamePattern,
				 const IB_STRING columnNamePattern)
{
  IB_Statement* statement = createStatement ();
 
  sprintf (queryStringBuffer_,  
	   "select" 
	   " P.RDB$PROCEDURE_NAME," 
	   " PP.RDB$PARAMETER_NAME," 
	   " PP.RDB$PARAMETER_TYPE,"
	   " F.RDB$FIELD_TYPE," 
	   " F.RDB$FIELD_SUB_TYPE," 
	   " F.RDB$FIELD_SCALE," 
	   " F.RDB$FIELD_LENGTH,"
	   " F.RDB$NULL_FLAG," 
	   " PP.RDB$DESCRIPTION "
	   "from" 
	   " RDB$PROCEDURES P," 
	   " RDB$PROCEDURE_PARAMETERS PP," 
	   " RDB$FIELDS F "
	   "where" 
	   " (P.RDB$PROCEDURE_NAME like '%s' or"
	   "  P.RDB$PROCEDURE_NAME like '%s %%') and"
	   " (PP.RDB$PARAMETER_NAME like '%s' or"
	   "  PP.RDB$PARAMETER_NAME like '%s %%') and"
	   " P.RDB$PROCEDURE_NAME = PP.RDB$PROCEDURE_NAME and"
	   " PP.RDB$FIELD_SOURCE = F.RDB$FIELD_NAME "
	   "order by" 
	   " P.RDB$PROCEDURE_NAME," 
	   " PP.RDB$PARAMETER_TYPE desc," 
	   " PP.RDB$PARAMETER_NUMBER",
	   procedureNamePattern,
	   procedureNamePattern,
	   columnNamePattern,
           columnNamePattern);

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_PROCEDURE_COLUMNS);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getTables (const IB_STRING tableNamePattern,
		       const IB_BOOLEAN* types)
{
  IB_Statement* statement = createStatement ();
  IB_BOOLEAN firstSelect = IB_TRUE;

  // Note: any changes to the SQL query must also be reflected in JIBSRemote

  char *position = queryStringBuffer_;

  // literals TABLE, SYSTEM TABLE, and VIEW are of different lengths, 
  // so they must be substituted for in JIBSRemote.cpp (use 'T', 'S', and 'V' here)

  if (types [SYSTEM_TABLE_TYPE]) {
    if (!firstSelect) {
      sprintf (position, " union ");
      position += strlen (position);
    }
    firstSelect = IB_FALSE;
    sprintf (position,  
	     "select"
	     " RDB$RELATION_NAME,"
	     " 'S',"
	     " RDB$DESCRIPTION,"
	     " RDB$OWNER_NAME "
	     "from"
	     " RDB$RELATIONS "
	     "where"
	     " RDB$SYSTEM_FLAG = 1 and"
	     " (RDB$RELATION_NAME like '%s' or"
	     "  RDB$RELATION_NAME like '%s %%') and"
	     " RDB$VIEW_SOURCE is null",
	     tableNamePattern,
             tableNamePattern);
    position += strlen (position);
  }

  if (types [TABLE_TYPE]) {
    if (!firstSelect) {
      sprintf (position, " union ");
      position += strlen (position);
    }
    firstSelect = IB_FALSE;
    sprintf (position,  
	     "select"
	     " RDB$RELATION_NAME,"
	     " 'T',"
	     " RDB$DESCRIPTION,"
	     " RDB$OWNER_NAME "
	     "from"
	     " RDB$RELATIONS "
	     "where"
	     " RDB$SYSTEM_FLAG = 0 and"
	     " (RDB$RELATION_NAME like '%s' or"
	     "  RDB$RELATION_NAME like '%s %%') and"
	     " RDB$VIEW_SOURCE is null",
	     tableNamePattern,
	     tableNamePattern);
    position += strlen (position);
  }

  if (types [VIEW_TYPE]) {
    if (!firstSelect) {
      sprintf (position, " union ");
      position += strlen (position);
    }
    firstSelect = IB_FALSE;
    sprintf (position,  
	     "select"
	     " RDB$RELATION_NAME,"
	     " 'V',"
	     " RDB$DESCRIPTION,"
	     " RDB$OWNER_NAME "
	     "from"
	     " RDB$RELATIONS "
	     "where"
	     " (RDB$RELATION_NAME like '%s' or"
	     "  RDB$RELATION_NAME like '%s %%') and"
	     " RDB$VIEW_SOURCE is not null",
	     tableNamePattern,
	     tableNamePattern);
    position += strlen (position);
  }

  sprintf (position, " order by 2, 1");

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_TABLES);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getTableTypes ()
{
  IB_Statement* statement = createStatement ();

  sprintf (queryStringBuffer_,  
           "select 'S' from RDB$DATABASE union "
	   "select 'T' from RDB$DATABASE union "
           "select 'V' from RDB$DATABASE "
           "order by 1");

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_TABLE_TYPES);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getColumns (const IB_STRING tableNamePattern,
			const IB_STRING columnNamePattern)
{
  IB_Statement* statement = createStatement ();
 
  sprintf (queryStringBuffer_,  
	   "select"
	   " RF.RDB$RELATION_NAME,"
	   " RF.RDB$FIELD_NAME,"
	   " F.RDB$FIELD_TYPE,"
	   " F.RDB$FIELD_SUB_TYPE,"
	   " F.RDB$FIELD_SCALE,"
	   " F.RDB$FIELD_LENGTH,"
	   " F.RDB$NULL_FLAG,"
	   " RF.RDB$DESCRIPTION,"
	   " RF.RDB$DEFAULT_SOURCE,"
	   " RF.RDB$FIELD_POSITION, "
	   " RF.RDB$NULL_FLAG, "
           " F.RDB$DEFAULT_SOURCE "
	   "from"
	   " RDB$RELATION_FIELDS RF,"
	   " RDB$FIELDS F "
	   "where"
	   " (RF.RDB$RELATION_NAME like '%s' or"
	   "  RF.RDB$RELATION_NAME like '%s %%') and"
	   " (RF.RDB$FIELD_NAME like '%s' or"
	   "  RF.RDB$FIELD_NAME like '%s %%') and"
	   " RF.RDB$FIELD_SOURCE = F.RDB$FIELD_NAME "
	   "order by 1, 10",
	   tableNamePattern, 
	   tableNamePattern,
	   columnNamePattern,
           columnNamePattern);

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_COLUMNS);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getColumnPrivileges (const IB_STRING table,
				 const IB_STRING columnNamePattern)
{
  IB_Statement* statement = createStatement ();
 
  sprintf (queryStringBuffer_,  
	"select "
            "RF.RDB$RELATION_NAME, "
            "RF.RDB$FIELD_NAME, "
            "UP.RDB$GRANTOR, "
            "UP.RDB$USER, "
            "UP.RDB$PRIVILEGE, "
            "UP.RDB$GRANT_OPTION "
        "from "
            "RDB$RELATION_FIELDS RF, "
            "RDB$FIELDS F, "
            "RDB$USER_PRIVILEGES UP "
        "where "
            "RF.RDB$RELATION_NAME = UP.RDB$RELATION_NAME and "
            "RF.RDB$FIELD_SOURCE = F.RDB$FIELD_NAME  and "
            "(UP.RDB$FIELD_NAME is null or "
             "UP.RDB$FIELD_NAME = RF.RDB$FIELD_NAME) and "
	    "UP.RDB$RELATION_NAME = '%s' and "
            "(RF.RDB$FIELD_NAME like '%s' or "
            "RF.RDB$FIELD_NAME like '%s %%' or "
            "RF.RDB$FIELD_NAME is null) and "
            "UP.RDB$OBJECT_TYPE = 0 "
	"order by 1, 2, 5 ",
   table, 
   columnNamePattern,
   columnNamePattern);

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_COLUMN_PRIVILEGES);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getTablePrivileges (const IB_STRING tableNamePattern)
{
  IB_Statement* statement = createStatement ();
 
  sprintf (queryStringBuffer_,  
	   "select"
	   " RDB$RELATION_NAME,"
	   " RDB$GRANTOR,"
	   " RDB$USER,"
	   " RDB$PRIVILEGE,"
	   " RDB$GRANT_OPTION "
	   "from" 
	   " RDB$USER_PRIVILEGES "
	   "where"
	   " (RDB$RELATION_NAME like '%s' or"
	   "  RDB$RELATION_NAME like '%s %%') and"
           " RDB$OBJECT_TYPE = 0 and"
           " RDB$FIELD_NAME is null "
	   "order by 1, 4",
	   tableNamePattern,
	   tableNamePattern);

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_TABLE_PRIVILEGES);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getBestRowIdentifier (const IB_STRING table, 
				  int scope, 
				  IB_BOOLEAN nullable)
{
  IB_Statement* statement = createStatement ();
 
  // for now at least, scope is ignored.
  // also nullable is ignored for now. If nullable is true, then we can include
  // columns that are nullable; if nullable is false, then we do not include
  // nullable columns in the result set, but (IB should not have best row identifiers
  // with null values in the columns, anyway).

  // Pseudo-code:
  // select primary keys 
  // if empty (there is no primary key) then 
  // select one and only one unique index 
  // (there could be multiple unique indices on a table) 

  // Refined the above pseudo-code: 
  // (select primary keys) 
  // union 
  // (select one and only one unique index where not exists (select primary keys)) 

  sprintf (queryStringBuffer_,
	   "select"
	   " RF.RDB$FIELD_NAME," 
	   " F.RDB$FIELD_TYPE,"
	   " F.RDB$FIELD_SUB_TYPE,"
	   " F.RDB$FIELD_SCALE,"
	   " F.RDB$FIELD_LENGTH "
	   "from"
	   " RDB$RELATION_CONSTRAINTS RC,"
	   " RDB$INDEX_SEGMENTS ISGMT,"
	   " RDB$RELATION_FIELDS RF,"
	   " RDB$FIELDS F "
	   "where"
           " RC.RDB$RELATION_NAME = '%s' and"
	   " RC.RDB$CONSTRAINT_TYPE = 'PRIMARY KEY' and"
	   " ISGMT.RDB$INDEX_NAME = RC.RDB$INDEX_NAME and"
	   " RF.RDB$FIELD_NAME = ISGMT.RDB$FIELD_NAME and"
	   " RF.RDB$RELATION_NAME = RC.RDB$RELATION_NAME and"
	   " F.RDB$FIELD_NAME = RF.RDB$FIELD_SOURCE "
	   "union all "
	   "select"
	   " RF.RDB$FIELD_NAME,"
	   " F.RDB$FIELD_TYPE,"
	   " F.RDB$FIELD_SUB_TYPE,"
	   " F.RDB$FIELD_SCALE,"
	   " F.RDB$FIELD_LENGTH "
	   "from"
	   " RDB$INDICES IND,"
	   " RDB$INDEX_SEGMENTS ISGMT,"
	   " RDB$RELATION_FIELDS RF,"
	   " RDB$FIELDS F "
	   "where not exists "
	   " (select * from"
           "   RDB$RELATION_CONSTRAINTS"
           " where"
           "   RDB$RELATION_NAME = '%s' and"
           "   RDB$CONSTRAINT_TYPE = 'PRIMARY KEY') and"
	   " IND.RDB$INDEX_NAME in"
           "  (select max(RDB$INDEX_NAME) from"
           "    RDB$INDICES"
           "   where"
           "    RDB$RELATION_NAME = '%s' and"
           "    RDB$UNIQUE_FLAG   = 1) and"
	   " ISGMT.RDB$INDEX_NAME = IND.RDB$INDEX_NAME and"
	   " RF.RDB$FIELD_NAME = ISGMT.RDB$FIELD_NAME and"
	   " RF.RDB$RELATION_NAME = IND.RDB$RELATION_NAME and"
	   " F.RDB$FIELD_NAME = RF.RDB$FIELD_SOURCE " 
	   "order by 1",
	   table,
	   table,
	   table);

  /*** Old buggy query: 
  // this SQL selects *all* fields from 
  // *any* index segment corresponding to a unique index.  
  sprintf (queryStringBuffer_,
	     "select"
	     " RF.RDB$FIELD_NAME,"
	     " F.RDB$FIELD_TYPE,"
	     " F.RDB$FIELD_SUB_TYPE,"
	     " F.RDB$FIELD_SCALE, "
	     " F.RDB$FIELD_LENGTH "
	     "from"
	     " RDB$INDICES IND,"
	     " RDB$INDEX_SEGMENTS ISG,"
	     " RDB$RELATION_FIELDS RF,"
	     " RDB$FIELDS F "
	     "where"
	     " IND.RDB$RELATION_NAME = '%s' and"
	     " IND.RDB$INDEX_NAME    = ISG.RDB$INDEX_NAME and"
	     " IND.RDB$UNIQUE_FLAG   = 1 and"
	     " ISG.RDB$FIELD_NAME    = RF.RDB$FIELD_NAME and"
	     " IND.RDB$RELATION_NAME = RF.RDB$RELATION_NAME and"
	     " F.RDB$FIELD_NAME      = RF.RDB$FIELD_SOURCE", 
	     table);
  ***/

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_BEST_ROW_IDENTIFIER);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getVersionColumns (const IB_STRING table)
{
  IB_Statement* statement = createStatement ();
 
  // Added 0=1 to force an empty result set.
  // BUG 9010 -- DatabaseMetaData.getVersionColumns() should always return empty result set 
  // According to the JDBC spec, this method is supposed to describe columns that 
  // change whenever any value in a row changes.  Current
  // InterClient implementation of this method describes columns that are COMPUTED BY fields, 
  // which differs from the spec. 
  // The JDBC description of this method is a bit pithy, 
  // but a more complete description of the getVersionColumns() method is found in the
  // description for SQLSpecialColumns in the ODBC 2.0 spec.  
  // The SQLSpecialColumns method in ODBC is like a combination of the
  // getBestRowIdentifier() and getVersionColumns() methods in JDBC.  
  // The description for its later functionality is as follows: 
  // SQL_ROWVER: returns the column or columns in the specified table, if any, 
  // that are automatically updated by the
  // datasource when any value in the row is updated by any transactions 
  // (as SQLBase ROWID or Sybase TIMESTAMP). 
  // Columns returned for column type SQL_ROWVER are useful for applications that need the 
  // ability to check whether any
  // columns in a given row have been updated while the row was reselected using the rowid. 
  // For example, after reselecting a
  // row using rowid, the application can compare the previous 
  // values in the SQL_ROWVER columns to the ones just fetched. If
  //  the value in a SQL_ROWVER column differs from the previous value, 
  // the application can alert the user that data on the
  // display has changed.
  // As far as I know, InterBase does not have a column or pseudo-column that is 
  // guaranteed to change with an update.  
  // Therefore, this method should always return an empty result set. 
  // Now, if you REALLY want a method that returns computed  columns, 
  // perhaps you can extend the DatabaseMetaData class. 
  // getComputedColumns() perhaps?  Don't do it on my account, though. 
  // --- Chris 
  
  // Except for the 0=1, this query returns computed-by columns
  sprintf (queryStringBuffer_,  
	   "select"
	   " RF.RDB$FIELD_NAME,"
	   " F.RDB$FIELD_TYPE,"
	   " F.RDB$FIELD_SUB_TYPE,"
	   " F.RDB$FIELD_SCALE,"
           " F.RDB$FIELD_LENGTH,"
	   " F.RDB$CHARACTER_LENGTH "
	   "from"
           " RDB$FIELDS F,"
	   " RDB$RELATION_FIELDS RF "
	   "where"
	   " 0 = 1 and "
           " RF.RDB$RELATION_NAME = '%s' and"
	   " RF.RDB$FIELD_SOURCE = F.RDB$FIELD_NAME and"
	   " F.RDB$COMPUTED_BLR IS NOT NULL",
	   table);

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_VERSION_COLUMNS);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getPrimaryKeys (const IB_STRING table)
{
  IB_Statement* statement = createStatement ();
 
  sprintf (queryStringBuffer_,  
	   "select"
	   " RC.RDB$RELATION_NAME,"
	   " ISGMT.RDB$FIELD_NAME,"
	   " ISGMT.RDB$FIELD_POSITION,"
	   " RC.RDB$CONSTRAINT_NAME "
	   "from"
	   " RDB$RELATION_CONSTRAINTS RC,"
	   " RDB$INDEX_SEGMENTS ISGMT "
	   "where"
	   " RC.RDB$RELATION_NAME = '%s' and"
	   " RC.RDB$INDEX_NAME = ISGMT.RDB$INDEX_NAME and"
           " RC.RDB$CONSTRAINT_TYPE = 'PRIMARY KEY' "
           "order by ISGMT.RDB$FIELD_NAME",
	   table);

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_PRIMARY_KEYS);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getImportedKeys (const IB_STRING table)
{
  IB_Statement* statement = createStatement ();
 
  sprintf (queryStringBuffer_,  
	   "select" 
	   " RELC_PRIM.RDB$RELATION_NAME,"    // prim.RDB$ key table name
	   " IS_PRIM.RDB$FIELD_NAME,"         // prim.RDB$ key column name
	   " RELC_FOR.RDB$RELATION_NAME,"     // foreign key table name
	   " IS_FOR.RDB$FIELD_NAME,"          // foreign key column name
	   " IS_FOR.RDB$FIELD_POSITION,"      // key sequence
	   " REFC_PRIM.RDB$UPDATE_RULE,"
	   " REFC_PRIM.RDB$DELETE_RULE,"
	   " RELC_FOR.RDB$CONSTRAINT_NAME,"   // foreign key constraint name
	   " RELC_PRIM.RDB$CONSTRAINT_NAME "  // primary key constraint name
	   "from"
	   " RDB$RELATION_CONSTRAINTS RELC_FOR,"
	   " RDB$REF_CONSTRAINTS REFC_FOR," 
	   " RDB$RELATION_CONSTRAINTS RELC_PRIM,"
	   " RDB$REF_CONSTRAINTS REFC_PRIM,"
	   " RDB$INDEX_SEGMENTS IS_PRIM,"
	   " RDB$INDEX_SEGMENTS IS_FOR "
	   "where"
	   " RELC_FOR.RDB$CONSTRAINT_TYPE = 'FOREIGN KEY' and"
	   " RELC_FOR.RDB$RELATION_NAME = '%s' and"
	   " RELC_FOR.RDB$CONSTRAINT_NAME = REFC_FOR.RDB$CONSTRAINT_NAME and"
	   " REFC_FOR.RDB$CONST_NAME_UQ = RELC_PRIM.RDB$CONSTRAINT_NAME and"
	   " RELC_PRIM.RDB$CONSTRAINT_TYPE = 'PRIMARY KEY' and" // useful check, anyay
	   " RELC_PRIM.RDB$INDEX_NAME = IS_PRIM.RDB$INDEX_NAME and"
	   " IS_FOR.RDB$INDEX_NAME = RELC_FOR.RDB$INDEX_NAME   and"
	   " IS_PRIM.RDB$FIELD_POSITION = IS_FOR.RDB$FIELD_POSITION  and"
	   " REFC_PRIM.RDB$CONSTRAINT_NAME = RELC_FOR.RDB$CONSTRAINT_NAME "
	   "order by" 
	   " RELC_PRIM.RDB$RELATION_NAME," 
	   " IS_FOR.RDB$FIELD_POSITION",
           table);

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_IMPORTED_KEYS);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getExportedKeys (const IB_STRING table)
{
  IB_Statement* statement = createStatement ();
 
  sprintf (queryStringBuffer_,  
	   "select" 
	   " RC_PRIM.RDB$RELATION_NAME,"   // prim.RDB$ key table name
	   " IS_PRIM.RDB$FIELD_NAME,"      // prim.RDB$ key column name
	   " RC_FOR.RDB$RELATION_NAME,"    // foreign key table name
	   " IS_FOR.RDB$FIELD_NAME,"       // foreign key column name
	   " IS_FOR.RDB$FIELD_POSITION,"   // key sequence
	   " REFC_PRIM.RDB$UPDATE_RULE,"   // if update or delete rule is null, interpret as RESTRICT 
	   " REFC_PRIM.RDB$DELETE_RULE,"
	   " RC_FOR.RDB$CONSTRAINT_NAME,"  // foreign key constraint name
	   " RC_PRIM.RDB$CONSTRAINT_NAME " // primary key constraint name
	   "from"
	   " RDB$RELATION_CONSTRAINTS RC_FOR,"
	   " RDB$REF_CONSTRAINTS REFC_FOR," 
	   " RDB$RELATION_CONSTRAINTS RC_PRIM,"
	   " RDB$REF_CONSTRAINTS REFC_PRIM,"
	   " RDB$INDEX_SEGMENTS IS_PRIM,"
	   " RDB$INDEX_SEGMENTS IS_FOR "
	   "where"
	   " RC_PRIM.RDB$CONSTRAINT_TYPE = 'PRIMARY KEY' and"
	   " RC_PRIM.RDB$RELATION_NAME = '%s' and"
	   " REFC_FOR.RDB$CONST_NAME_UQ = RC_PRIM.RDB$CONSTRAINT_NAME and"
	   " RC_FOR.RDB$CONSTRAINT_NAME = REFC_FOR.RDB$CONSTRAINT_NAME and"
	   " RC_FOR.RDB$CONSTRAINT_TYPE = 'FOREIGN KEY' and" // useful check, anyay
	   " RC_PRIM.RDB$INDEX_NAME = IS_PRIM.RDB$INDEX_NAME and"
	   " IS_FOR.RDB$INDEX_NAME = RC_FOR.RDB$INDEX_NAME   and"
	   " IS_PRIM.RDB$FIELD_POSITION = IS_FOR.RDB$FIELD_POSITION  and"
	   " REFC_PRIM.RDB$CONSTRAINT_NAME = RC_FOR.RDB$CONSTRAINT_NAME "
	   "order by"
	   " RC_FOR.RDB$RELATION_NAME," 
	   " IS_FOR.RDB$FIELD_POSITION", 
	   table);


  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_EXPORTED_KEYS);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getCrossReference (const IB_STRING primaryTable, 
			       const IB_STRING foreignTable)
{
  IB_Statement* statement = createStatement ();
 
  sprintf (queryStringBuffer_,  
	   "select"
	   " RC_PRIM.RDB$RELATION_NAME,"    // prim.RDB$ key table name 
	   " IS_PRIM.RDB$FIELD_NAME,"       // prim.RDB$ key column name 
	   " RC_FOR.RDB$RELATION_NAME,"     // foreign key table name 
	   " IS_FOR.RDB$FIELD_NAME,"        // foreign key column name 
	   " IS_FOR.RDB$FIELD_POSITION,"    // key sequence 
	   " REFC_PRIM.RDB$UPDATE_RULE,"    // if update or delete rule is null, interpret as RESTRICT
	   " REFC_PRIM.RDB$DELETE_RULE,"
	   " RC_FOR.RDB$CONSTRAINT_NAME,"   // foreign key constraint name 
	   " RC_PRIM.RDB$CONSTRAINT_NAME "  // primary key constraint name 
	   "from"
	   " RDB$RELATION_CONSTRAINTS RC_FOR,"
	   " RDB$REF_CONSTRAINTS REFC_FOR," 
	   " RDB$RELATION_CONSTRAINTS RC_PRIM,"
	   " RDB$REF_CONSTRAINTS REFC_PRIM,"
	   " RDB$INDEX_SEGMENTS IS_PRIM,"
	   " RDB$INDEX_SEGMENTS IS_FOR "
	   "where"
	   " RC_PRIM.RDB$RELATION_NAME = '%s' and"
	   " RC_FOR.RDB$RELATION_NAME = '%s' and"
	   " RC_PRIM.RDB$CONSTRAINT_TYPE = 'PRIMARY KEY' and"
	   " REFC_FOR.RDB$CONST_NAME_UQ = RC_PRIM.RDB$CONSTRAINT_NAME and"
	   " RC_FOR.RDB$CONSTRAINT_NAME = REFC_FOR.RDB$CONSTRAINT_NAME and"
	   " RC_FOR.RDB$CONSTRAINT_TYPE = 'FOREIGN KEY' and" // useful check, anyay 
	   " RC_PRIM.RDB$INDEX_NAME = IS_PRIM.RDB$INDEX_NAME and"
	   " IS_FOR.RDB$INDEX_NAME = RC_FOR.RDB$INDEX_NAME and"
	   " IS_PRIM.RDB$FIELD_POSITION = IS_FOR.RDB$FIELD_POSITION and"
	   " REFC_PRIM.RDB$CONSTRAINT_NAME = RC_FOR.RDB$CONSTRAINT_NAME "
	   "order by"
	   " RC_PRIM.RDB$RELATION_NAME,"
	   " IS_FOR.RDB$FIELD_POSITION", 
           primaryTable, 
           foreignTable);


  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_CROSS_REFERENCE);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getIndexInfo (const IB_STRING table, 
			  const IB_BOOLEAN unique,
			  const IB_BOOLEAN approximate)
{
  IB_Statement* statement = createStatement ();
  char queryStringBufferPart2[512];

  sprintf (queryStringBuffer_,  
	   "select"
	   " I.RDB$RELATION_NAME,"
	   " I.RDB$UNIQUE_FLAG,"
	   " I.RDB$INDEX_NAME,"
	   " ISGMT.RDB$FIELD_POSITION,"
	   " ISGMT.RDB$FIELD_NAME,"
           " I.RDB$INDEX_TYPE,"
	   " I.RDB$SEGMENT_COUNT,"
	   " COUNT (DISTINCT P.RDB$PAGE_NUMBER) "
	   "from"
	   " RDB$INDICES I,"
	   " RDB$INDEX_SEGMENTS ISGMT,"
	   " RDB$PAGES P,"
	   " RDB$RELATIONS R "
	   "where");

  if (unique)
    strcat (queryStringBuffer_,
	   " I.RDB$UNIQUE_FLAG = 1 AND ");

  sprintf (queryStringBufferPart2, 
	   " I.RDB$RELATION_NAME = '%s'"
	   " AND I.RDB$INDEX_NAME = ISGMT.RDB$INDEX_NAME"
	   " AND P.RDB$RELATION_ID = R.RDB$RELATION_ID"
	   " AND R.RDB$RELATION_NAME = I.RDB$RELATION_NAME"
	   " AND (P.RDB$PAGE_TYPE = 7 OR P.RDB$PAGE_TYPE = 6) "
           "group by"
           " I.RDB$INDEX_NAME,"
           " I.RDB$RELATION_NAME,"
           " I.RDB$UNIQUE_FLAG, "
           " ISGMT.RDB$FIELD_POSITION, "
           " ISGMT.RDB$FIELD_NAME, "
           " I.RDB$INDEX_TYPE, "
           " I.RDB$SEGMENT_COUNT "
	   "order by 2,3,4",
           table);

  strcat (queryStringBuffer_, queryStringBufferPart2);

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_INDEX_INFO);
  resultSet->open ();
  return statement;
}

IB_Statement*
IB_Catalog::getTypeInfo ()
{
  IB_Statement* statement = createStatement ();

  // !!! order by 2
  sprintf (queryStringBuffer_,  
	   "select 1, -7 from RDB$DATABASE union "  // SMALLINT_TYPE, BIT_SQLTYPE
	   "select 1, -6 from RDB$DATABASE union "  // SMALLINT_TYPE, TINYINT_SQLTYPE
	   "select 1, 5 from RDB$DATABASE union "   // SMALLINT_TYPE, SMALLINT_SQLTYPE
	   "select 2, 4 from RDB$DATABASE union "   // INTEGER_TYPE, INTEGER_SQLTYPE
	   "select 3, 7 from RDB$DATABASE union "   // FLOAT_TYPE, REAL_SQLTYPE
	   "select 4, 8 from RDB$DATABASE union "   // DOUBLE_TYPE, DOUBLE_SQLTYPE
	   "select 4, -5 from RDB$DATABASE union "  // DOUBLE_TYPE, BIGINT_SQLTYPE
	   "select 4, 6 from RDB$DATABASE union "   // DOUBLE_TYPE, FLOAT_SQLTYPE
	   "select 5, 2 from RDB$DATABASE union "   // NUMERIC_DOUBLE_TYPE, NUMERIC_SQLTYPE
	   "select 5, 3 from RDB$DATABASE union "   // NUMERIC_DOUBLE_TYPE, DECIMAL_SQLTYPE
	   "select 8, 1 from RDB$DATABASE union "   // CHAR_TYPE, CHAR_SQLTYPE
	   "select 9, 12 from RDB$DATABASE union "  // VARCHAR_TYPE, VARCHAR_SQLTYPE
	   "select 10, -1 from RDB$DATABASE union " // CLOB_TYPE, LONGVARCHAR_SQLTYPE
	   "select 11, 91 from RDB$DATABASE union " // DATE_TYPE, DATE_SQLTYPE
	   "select 11, 92 from RDB$DATABASE union " // DATE_TYPE, TIME_SQLTYPE
	   "select 11, 93 from RDB$DATABASE union " // DATE_TYPE, TIMESTAMP_SQLTYPE
	   "select 12, -2 from RDB$DATABASE union " // BLOB_TYPE, BINARY_SQLTYPE
	   "select 12, -3 from RDB$DATABASE union " // BLOB_TYPE, VARBINARY_SQLTYPE
	   "select 12, -4 from RDB$DATABASE");      // BLOB_TYPE, LONGVARBINARY_SQLTYPE

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->setCatalogFunction (CATALOG_GET_TYPE_INFO);
  resultSet->open ();
  return statement;
}

// Note: this won't be called if you're sysdba
IB_BOOLEAN
IB_Catalog::allProceduresAreCallable (const IB_STRING user)
{
  IB_Statement* statement = createStatement ();

  sprintf (queryStringBuffer_,  
	   "select"
	   " P.RDB$PROCEDURE_NAME "
	   "from"
	   " RDB$PROCEDURES P "
           "where"
	   " not exists"
	   " ("
	   "  select"
	   "   UP.RDB$RELATION_NAME"
	   "  from"
	   "   RDB$USER_PRIVILEGES UP"
           "  where"
	   "   (UP.RDB$USER = '%s' or UP.RDB$USER = 'PUBLIC') and "
           "   UP.RDB$RELATION_NAME = P.RDB$PROCEDURE_NAME and"
           "   UP.RDB$OBJECT_TYPE = 5 and"
           "   UP.RDB$PRIVILEGE = 'X'"
	   " )", 
           user);


  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->open ();
  IB_BOOLEAN result = !resultSet->next ();
  resultSet->close ();
  statement->close ();
  return result;
}

// Note: this won't be called if you're sysdba
IB_BOOLEAN
IB_Catalog::allTablesAreSelectable (const IB_STRING user)
{
  IB_Statement* statement = createStatement ();

  sprintf (queryStringBuffer_,  
	" select "
	   " REL.RDB$RELATION_NAME "
	" from"
	   " RDB$RELATIONS REL "
	" where "
	   " (REL.RDB$SYSTEM_FLAG = 0) and "
	   " not exists ( "
	   " select "
	       " UP.RDB$RELATION_NAME "
	   " from "
	       " RDB$USER_PRIVILEGES UP "
           " where"
	       " UP.RDB$OBJECT_TYPE = 0 and"  // denotes procedure
               " UP.RDB$USER_TYPE = 8 and"      // denotes a user
	       " UP.RDB$RELATION_NAME = REL.RDB$RELATION_NAME and "
               " (RDB$USER = '%s' or RDB$USER = 'PUBLIC') and"
               " (RDB$PRIVILEGE IN ('S',  'A')))", 
           user);

  IB_ResultSet* resultSet = statement->prepareNoInput (queryStringBuffer_);
  resultSet->open ();
  IB_BOOLEAN result = !resultSet->next ();
  resultSet->close ();
  statement->close ();
  return result;
}

IB_LDString
IB_Catalog::getIBTypeName (const IB_Types::IBType ibType)
{
  switch (ibType) {
  case IB_Types::SMALLINT_TYPE:
    return IB_LDString ((IB_SSHORT16) strlen (smallintName__), (IB_STRING) smallintName__);
  case IB_Types::INTEGER_TYPE:
    return IB_LDString ((IB_SSHORT16) strlen (integerName__),(IB_STRING)  integerName__);
  case IB_Types::FLOAT_TYPE:
    return IB_LDString ((IB_SSHORT16) strlen (floatName__), (IB_STRING) floatName__);
  case IB_Types::DOUBLE_TYPE:
    return IB_LDString ((IB_SSHORT16) strlen (doubleName__), (IB_STRING) doubleName__);
  case IB_Types::NUMERIC_DOUBLE_TYPE:
  case IB_Types::NUMERIC_INTEGER_TYPE:
  case IB_Types::NUMERIC_SMALLINT_TYPE:
    return IB_LDString ((IB_SSHORT16) strlen (numericName__), (IB_STRING) numericName__);
  case IB_Types::CHAR_TYPE:
    return IB_LDString ((IB_SSHORT16) strlen (charName__), (IB_STRING) charName__);
  case IB_Types::VARCHAR_TYPE:
    return IB_LDString ((IB_SSHORT16) strlen (varCharName__), (IB_STRING) varCharName__);
  case IB_Types::DATE_TYPE:
    return IB_LDString ((IB_SSHORT16) strlen (dateName__), (IB_STRING) dateName__);
  case IB_Types::CLOB_TYPE:
  case IB_Types::BLOB_TYPE:
    return IB_LDString ((IB_SSHORT16) strlen (blobName__), (IB_STRING) blobName__);
  case IB_Types::ARRAY_TYPE:
    return IB_LDString ((IB_SSHORT16) strlen (arrayName__), (IB_STRING) arrayName__);
  default:
    return NULL;
  }
}    

IB_BOOLEAN
IB_Catalog::isUnsigned (const IB_Types::IBType ibType)
{
  switch (ibType) {
  case IB_Types::SMALLINT_TYPE: 
  case IB_Types::INTEGER_TYPE: 
  case IB_Types::DOUBLE_TYPE: 
  case IB_Types::FLOAT_TYPE: 
  case IB_Types::NUMERIC_SMALLINT_TYPE: 
  case IB_Types::NUMERIC_INTEGER_TYPE: 
  case IB_Types::NUMERIC_DOUBLE_TYPE: 
    return (IB_FALSE);
  case IB_Types::CHAR_TYPE: 
  case IB_Types::VARCHAR_TYPE: 
  case IB_Types::DATE_TYPE: 
  case IB_Types::CLOB_TYPE: 
  case IB_Types::BLOB_TYPE: 
  case IB_Types::ARRAY_TYPE: 
    return (IB_TRUE);
  case IB_Types::NULL_TYPE:
  default:
    return (IB_TRUE);      // !!! BUG CHECK
  }
}

IB_Catalog::SQLType
IB_Catalog::getSQLType (const IB_Types::IBType ibType)
{
  switch (ibType) {
  case IB_Types::SMALLINT_TYPE: 
    return SMALLINT_SQLTYPE;
  case IB_Types::INTEGER_TYPE: 
    return INTEGER_SQLTYPE;
  case IB_Types::DOUBLE_TYPE: 
    return DOUBLE_SQLTYPE;
  case IB_Types::FLOAT_TYPE: 
    return FLOAT_SQLTYPE;
  case IB_Types::CHAR_TYPE: 
    return CHAR_SQLTYPE;
  case IB_Types::VARCHAR_TYPE: 
    return VARCHAR_SQLTYPE;
  case IB_Types::DATE_TYPE: 
    return TIMESTAMP_SQLTYPE;
  case IB_Types::CLOB_TYPE: 
      return LONGVARCHAR_SQLTYPE;
  case IB_Types::BLOB_TYPE: 
      return LONGVARBINARY_SQLTYPE;
  case IB_Types::ARRAY_TYPE: 
    return OTHER_SQLTYPE;
  case IB_Types::NUMERIC_SMALLINT_TYPE: 
  case IB_Types::NUMERIC_INTEGER_TYPE: 
  case IB_Types::NUMERIC_DOUBLE_TYPE: 
    return NUMERIC_SQLTYPE;
  case IB_Types::NULL_TYPE:
  default:
    return NULL_SQLTYPE;      // !!! BUG CHECK
  }
}

int
IB_Catalog::getMaxPrecision (const IB_Types::IBType ibType)
{
  switch (ibType) {
  case IB_Types::SMALLINT_TYPE: 
  case IB_Types::NUMERIC_SMALLINT_TYPE: 
  return 5;
  case IB_Types::INTEGER_TYPE: 
  case IB_Types::NUMERIC_INTEGER_TYPE: 
    return 10;
  case IB_Types::DOUBLE_TYPE: 
   case IB_Types::NUMERIC_DOUBLE_TYPE: 
   return 15;
  case IB_Types::FLOAT_TYPE: 
    return 7;
  case IB_Types::CHAR_TYPE: 
  case IB_Types::VARCHAR_TYPE: 
    return 32664;
  case IB_Types::DATE_TYPE: 
    return 19;
  case IB_Types::CLOB_TYPE: 
  case IB_Types::BLOB_TYPE: 
  case IB_Types::ARRAY_TYPE: 
    return 0;
  case IB_Types::NULL_TYPE:
  default:
    return NULL_SQLTYPE;      // !!! BUG CHECK
  }
}

IB_STRING
IB_Catalog::getLiteralPrefix (const IB_Types::IBType ibType)
{
  switch (ibType) {
  case IB_Types::CHAR_TYPE: 
  case IB_Types::VARCHAR_TYPE: 
  case IB_Types::DATE_TYPE: 
    return "'";

  case IB_Types::SMALLINT_TYPE: 
  case IB_Types::INTEGER_TYPE: 
  case IB_Types::DOUBLE_TYPE: 
  case IB_Types::FLOAT_TYPE: 
  case IB_Types::CLOB_TYPE: 
  case IB_Types::BLOB_TYPE: 
  case IB_Types::ARRAY_TYPE: 
  case IB_Types::NUMERIC_SMALLINT_TYPE: 
  case IB_Types::NUMERIC_INTEGER_TYPE: 
  case IB_Types::NUMERIC_DOUBLE_TYPE: 
  case IB_Types::NULL_TYPE:
  default:
    return NULL;
  }
}

IB_STRING
IB_Catalog::getLiteralSuffix (const IB_Types::IBType ibType)
{
  switch (ibType) {
  case IB_Types::CHAR_TYPE: 
  case IB_Types::VARCHAR_TYPE: 
  case IB_Types::DATE_TYPE: 
    return "'";

  case IB_Types::SMALLINT_TYPE: 
  case IB_Types::INTEGER_TYPE: 
  case IB_Types::DOUBLE_TYPE: 
  case IB_Types::FLOAT_TYPE: 
  case IB_Types::CLOB_TYPE: 
  case IB_Types::BLOB_TYPE: 
  case IB_Types::ARRAY_TYPE: 
  case IB_Types::NUMERIC_SMALLINT_TYPE: 
  case IB_Types::NUMERIC_INTEGER_TYPE: 
  case IB_Types::NUMERIC_DOUBLE_TYPE: 
  case IB_Types::NULL_TYPE:
  default:
    return NULL;
  }
}

IB_BOOLEAN
IB_Catalog::isCaseSensitive (const IB_Types::IBType ibType)
{
  switch (ibType) {
  case IB_Types::CHAR_TYPE: 
  case IB_Types::VARCHAR_TYPE: 
  case IB_Types::DATE_TYPE: 
    return IB_TRUE;

  case IB_Types::SMALLINT_TYPE: 
  case IB_Types::INTEGER_TYPE: 
  case IB_Types::DOUBLE_TYPE: 
  case IB_Types::FLOAT_TYPE: 
  case IB_Types::CLOB_TYPE: 
  case IB_Types::BLOB_TYPE: 
  case IB_Types::ARRAY_TYPE: 
  case IB_Types::NUMERIC_SMALLINT_TYPE: 
  case IB_Types::NUMERIC_INTEGER_TYPE: 
  case IB_Types::NUMERIC_DOUBLE_TYPE: 
  case IB_Types::NULL_TYPE:
  default:
    return IB_FALSE;
  }
}

IB_BOOLEAN
IB_Catalog::maybeMoneyValue (const IB_Types::IBType ibType)
{
  switch (ibType) {
  case IB_Types::SMALLINT_TYPE: 
  case IB_Types::INTEGER_TYPE: 
    return IB_TRUE;

  case IB_Types::CHAR_TYPE: 
  case IB_Types::VARCHAR_TYPE: 
  case IB_Types::DATE_TYPE: 
  case IB_Types::DOUBLE_TYPE: 
  case IB_Types::FLOAT_TYPE: 
  case IB_Types::CLOB_TYPE: 
  case IB_Types::BLOB_TYPE: 
  case IB_Types::ARRAY_TYPE: 
  case IB_Types::NUMERIC_SMALLINT_TYPE: 
  case IB_Types::NUMERIC_INTEGER_TYPE: 
  case IB_Types::NUMERIC_DOUBLE_TYPE: 
  case IB_Types::NULL_TYPE:
  default:
    return IB_FALSE;
  }
}

short
IB_Catalog::getMinScale (const IB_Types::IBType ibType)
{
  switch (ibType) {
  case IB_Types::SMALLINT_TYPE: 
  case IB_Types::INTEGER_TYPE: 
  case IB_Types::CHAR_TYPE: 
  case IB_Types::VARCHAR_TYPE: 
  case IB_Types::DATE_TYPE: 
  case IB_Types::DOUBLE_TYPE: 
  case IB_Types::FLOAT_TYPE: 
  case IB_Types::CLOB_TYPE: 
  case IB_Types::BLOB_TYPE: 
  case IB_Types::ARRAY_TYPE: 
  case IB_Types::NUMERIC_SMALLINT_TYPE: 
  case IB_Types::NUMERIC_INTEGER_TYPE: 
  case IB_Types::NUMERIC_DOUBLE_TYPE: 
  case IB_Types::NULL_TYPE:
  default:
    return 0;
  }
}

short
IB_Catalog::getMaxScale (const IB_Types::IBType ibType)
{
  switch (ibType) {
  case IB_Types::SMALLINT_TYPE: 
    return 5;
  case IB_Types::INTEGER_TYPE: 
    return 10;

  case IB_Types::CHAR_TYPE: 
  case IB_Types::VARCHAR_TYPE: 
  case IB_Types::DATE_TYPE: 
  case IB_Types::DOUBLE_TYPE: 
  case IB_Types::FLOAT_TYPE: 
  case IB_Types::CLOB_TYPE: 
  case IB_Types::BLOB_TYPE: 
  case IB_Types::ARRAY_TYPE: 
  case IB_Types::NUMERIC_SMALLINT_TYPE: 
  case IB_Types::NUMERIC_INTEGER_TYPE: 
  case IB_Types::NUMERIC_DOUBLE_TYPE: 
  case IB_Types::NULL_TYPE:
  default:
    return 0;
  }
}
 
// !!! this should be rewritten to call blob.getString()
IB_STRING
IB_Catalog::getDefaultValueFromBlob (const IB_BLOBID blobId)
{
  /*
  IB_Statement *statement = createStatement ();
  IB_Blob *blob = new IB_Blob (*statement, 80, 10, blobId);

  // !!! big performance problem here.
  IB_BUFF_PTR defaultValueSegment   = new IB_BUFF_CHAR [DEFAULT_VALUE_BUFFER_LEN];
  IB_BUFF_PTR defaultValue          = new IB_BUFF_CHAR [DEFAULT_VALUE_BUFFER_LEN];
  IB_BUFF_PTR defaultValueToReturn  = new IB_BUFF_CHAR [DEFAULT_VALUE_BUFFER_LEN];
  // the caller must free defaultValueToReturn.

  IB_BUFF_PTR keywordDefault;
  

  int defaultValueLength = 0;
  IB_BOOLEAN retValue;

  *defaultValue = 0;

  blob->open();

  // get as many segments as possible, with a maximum of
  // DEFAULT_VALUE_BUFFER_LEN characters.
  while (IB_TRUE) {
    retValue = blob->get(defaultValueSegment);
  // !!! big performance problem here.
    defaultValueLength = defaultValueLength + strlen (defaultValueSegment);
    if (defaultValueLength >= DEFAULT_VALUE_BUFFER_LEN)
      break;
    strcat (defaultValue, defaultValueSegment);
    if (!retValue)
      break;
  } 
 
  blob->close();


  // The string defaultValue is of the form: default <actual_default_value>
  // We need to step over the keyword default if it of this form.
  keywordDefault = strstr(defaultValue, "default");

  if (keywordDefault)
    strcpy (defaultValueToReturn, keywordDefault+8);
    // 8 = strlen("defaultValue")+1 (for the space that is after the word default.)
  else
    strcpy (defaultValueToReturn, defaultValue);

  delete defaultValueSegment;
  delete defaultValue;

  return ((IB_STRING) (defaultValueToReturn) );
  */
  return 0;
}

