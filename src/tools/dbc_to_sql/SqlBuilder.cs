/*
    Copyright 2018-2019 KeycapEmu

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

using System;
using System.Collections.Generic;
using System.Text;
using System.Xml;

namespace dbc_to_sql
{
    /// <summary>
    /// Builds a .sql file from a Dbc
    /// </summary>
    public class SqlBuilder
    {
        public SqlBuilder()
        {
        }

        /// <summary>
        /// Returns the SQL string to create and populate the given Dbc
        /// </summary>
        /// <param name="dbc">The Dbc to build as a SQL file</param>
        /// <returns>SQL string to create and populate the given Dbc</returns>
        public string Build(Dbc dbc)
        {
            var sql = new StringBuilder();

            sql.AppendFormat("DROP TABLE IF EXISTS `{0}`;\n", dbc.Name);
            sql.AppendFormat("CREATE TABLE `{0}` (\n", dbc.Name);

            foreach (Dbc.Coloumn coloumn in dbc[0])
                buildColoumnHead(ref sql, coloumn);

            // TODO: Super hacky - remove the last comma added
            sql.Remove(sql.Length - 2, 2);

            buildPrimaryKeys(ref sql, dbc);
            buildForeignKeys(ref sql, dbc);

            sql.Append(");\n\n");

            buildEntries(ref sql, dbc);

            return sql.ToString();
        }

        private string[] localizations =
        {
            "enUS",
            "koKR",
            "frFR",
            "deDE",
            "zhCN",
            "zhTW",
            "esES",
            "esMX",
            "ruRU",
            "jaJP",
            "ptPT",
            "itIT",
            "unk1",
            "unk2",
            "unk3",
            "unk4",
        };

        /// <summary>
        /// Creates the coloumn description
        /// </summary>
        /// <param name="sql">StringBuilder to write to</param>
        /// <param name="coloumn">Dbc.Coloumn to describe</param>
        private void buildColoumnHead(ref StringBuilder sql, Dbc.Coloumn coloumn)
        {
            if (coloumn.Value is LocalizedString)
            {
                var localizedString = coloumn.Value as LocalizedString;
                int i = 0;
                foreach (var str in localizedString.Strings)
                {
                    sql.AppendFormat("    `{0}_{1}` TEXT NOT NULL,\n", coloumn.Name, localizations[i]);
                    ++i;
                }

                sql.AppendFormat("    `{0}_flags` INT UNSIGNED NOT NULL,\n", coloumn.Name);
                return;
            }
            sql.AppendFormat("    `{0}` {1} {2}NULL,\n", coloumn.Name, toSqlType(coloumn.Type), coloumn.ZeroValues.Count > 0 ? "" : "NOT ");
        }

        /// <summary>
        /// Creates the primary keys
        /// </summary>
        /// <param name="sql">StringBuilder to write to</param>
        /// <param name="dbc">Dbc containing the primary keys</param>
        private void buildPrimaryKeys(ref StringBuilder sql, Dbc dbc)
        {
            if (dbc.PrimaryKeys.Count <= 0)
                return;

            sql.Append(",\n    PRIMARY KEY(");

            bool first = true;
            foreach (string primaryKey in dbc.PrimaryKeys)
            {
                if (!first)
                    sql.Append(", ");

                sql.AppendFormat("`{0}`", primaryKey);
                first = false;
            }

            sql.Append(")");
        }

        /// <summary>
        /// Creates the foreign keys
        /// </summary>
        /// <param name="sql">StringBuilder to write to</param>
        /// <param name="dbc">Dbc containing the foreign keys</param>
        private void buildForeignKeys(ref StringBuilder sql, Dbc dbc)
        {
            if (dbc.ForeignKeys.Count <= 0)
            {
                return;
            }

            sql.Append(",\n");

            for (int i = 0; i < dbc.ForeignKeys.Count; ++i)
            {
                var key = dbc.ForeignKeys[i];
                sql.AppendFormat("    FOREIGN KEY (`{0}`)\n", key.ColoumnName);
                sql.AppendFormat("      REFERENCES `{0}`(`{1}`)\n", key.TableName, key.ForeignColoumnName);
                sql.Append("      ON UPDATE CASCADE\n");
                sql.Append("      ON DELETE CASCADE");

                if (i < dbc.ForeignKeys.Count - 1)
                    sql.Append(",\n");
                else
                    sql.Append("\n");
            }
        }

        /// <summary>
        /// Creates the lines populating the table
        /// </summary>
        /// <param name="sql">StringBuilder to write to</param>
        /// <param name="dbc">Dbc containing the entries</param>
        private void buildEntries(ref StringBuilder sql, Dbc dbc)
        {
            sql.AppendFormat("INSERT INTO `{0}` VALUES \n", dbc.Name);

            for (int i = 0; i < dbc.Rows; ++i)
            {
                var coloumns = dbc[i];
                buildColoumns(ref sql, coloumns);

                if (i < dbc.Rows - 1)
                    sql.Append(",\n");
            }

            sql.Append(";\n");
        }

        /// <summary>
        /// Writes a single row coloumn by coloumn
        /// </summary>
        /// <param name="sql">StringBuilder to write to</param>
        /// <param name="coloumns">The coloumns to write</param>
        private void buildColoumns(ref StringBuilder sql, List<Dbc.Coloumn> coloumns)
        {
            sql.Append("( ");

            for (int i = 0; i < coloumns.Count; ++i)
            {
                var coloumn = coloumns[i];
                if (coloumn.IsNull)
                    sql.AppendFormat("NULL{0}", i == coloumns.Count - 1 ? "" : ", ");
                else
                    sql.AppendFormat("'{0}'{1}", coloumn.Value.ToString(), i == coloumns.Count - 1 ? "" : ", ");
            }

            sql.Append(" )");
        }

        /// <summary>
        /// Converts a C# Type to a SQL type
        /// </summary>
        /// <param name="type">The C# Type to convert</param>
        /// <returns>The corresponding SQL type</returns>
        private string toSqlType(Type type)
        {
            switch (type.Name)
            {
                default: return "UNSUPPORTED " + type.Name;
                case "Byte": return "TINYINT";
                case "UInt32": return "INT UNSIGNED";
                case "Int32": return "INT";
                case "String": return "TEXT";
                case "Single": return "FLOAT";
            }
        }
    }
}