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
package interbase.interclient;

/**
 * An enumeration of InterClient error codes.
 * This enumeration contains all error codes
 * for SQL exceptions generated by the driver itself.
 * SQL exceptions which originate from within the
 * InterBase engine will use the standard
 * isc error codes which are found in the InterBase
 * <code>ibase.h</code> include file.
 * <p>
 * Also see <a href="../../../help/icSQLStates.html">SQL States</a>.
 *
 * @author Paul Ostler
 * @since <font color=red>Extension, since InterClient 1.50</font>
 **/
public final class ErrorCodes
{
  // This is a static class, so define a private
  // constructor so that the default constructor
  // is not automatically exported.
  ErrorCodes () {}

  // ************************************
  // ***   There is one code for each ***
  // ***   exception subclass, but    ***
  // ***   only some are public       ***
  // ************************************
  public static final int badInstallation = 6;
  public static final int bugCheck = 9;
  public static final int characterEncoding = 3;
  public static final int communication = 1;
  public static final int driverNotCapable = 7;
  public static final int invalidOperation = 2;
  public static final int missingResourceBundle = 12;
  public static final int outOfMemory = 11;
  public static final int remoteProtocol = 10;
  public static final int socketTimeout = 4;
  public static final int unknownHost = 5;

  static final int unsupportedCharacterSet__ = 128;
  static final int invalidArgument__ = 129;
// CJL-IB6 added for dialect support
// !!! perhaps this is unnecessary because we can use invalidArgument__ for
// the SQL Dialect exception.  
  static final int unsupportedSQLDialectException__ = 130;
// CJL-IB6 end

  // ************************************************
  // *** InterBase error codes only, from ibase.h ***
  // ************************************************

  static final int isc_virmemexh                  = 335544430;
  static final int isc_bug_check                  = 335544333;
  static final int isc_unavailable                = 335544375;
  static final int isc_io_error                   = 335544344;
  static final int isc_lock_conflict              = 335544345;
  static final int isc_update_conflict            = 335544451;
  static final int isc_deadlock                   = 335544336;
  static final int isc_db_corrupt                 = 335544335;
  static final int isc_badpage                    = 335544405;
  static final int isc_bufexh                     = 335544389;
  static final int isc_bad_checksum               = 335544649;
  static final int isc_login                      = 335544472;
}

