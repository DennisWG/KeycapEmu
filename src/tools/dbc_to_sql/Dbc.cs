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
using System.Collections;
using System.Collections.Generic;

namespace dbc_to_sql
{
    /// <summary>
    /// Represents a .dbc from World of Warcraft
    /// </summary>
    public class Dbc : IEnumerable
    {
        /// <summary>
        /// A coloumn stored within a Dbc
        /// </summary>
        public class Coloumn
        {
            public Coloumn()
            {
            }

            public Coloumn(object value, Type type, string name)
            {
                Value = value;
                Type = type;
                Name = name;
            }

            /// <summary>
            /// The value stored
            /// </summary>
            public object Value { get; set; }

            /// <summary>
            /// The type stored
            /// </summary>
            /// <value></value>
            public Type Type { get; set; }

            /// <summary>
            /// The coloumn's name
            /// </summary>
            public string Name { get; set; }

            /// <summary>
            /// A set of values that will create an output of NULL if encountered within the Dbc
            /// </summary>
            public List<string> ZeroValues { get; set; } = new List<string>();

            /// <summary>
            /// Indicates wether this value should be ouputted as NULL in the resulting .sql
            /// </summary>
            public bool IsNull { get; set; } = false;
        }

        /// <summary>
        /// A key refering to another Dbc's entry
        /// </summary>
        public class ForeignKey
        {
            public ForeignKey(string coloumnName, string tableName, string foreignColoumnName)
            {
                ColoumnName = coloumnName;
                TableName = tableName;
                ForeignColoumnName = foreignColoumnName;
            }

            /// <summary>
            /// The name of the coloumn referring to a foreign table
            /// </summary>
            public string ColoumnName { get; }

            /// <summary>
            /// The name of the table this key is referring to
            /// </summary>
            public string TableName { get; }

            /// <summary>
            /// The name of the foreign coloumn this key is referring to
            /// </summary>
            public string ForeignColoumnName { get; }

        }

        /// <summary>
        /// Returns the number of coloumns within the Dbc
        /// </summary>
        public int Coloumns
        {
            get
            {
                return Entries[0].Count;
            }
        }

        /// <summary>
        /// Returns the number of rows within the Dbc
        /// </summary>
        public int Rows
        {
            get
            {
                return Entries.Count;
            }
        }

        /// <summary>
        /// The Dbc's name
        /// </summary>
        public string Name { get; set; }

        /// <summary>
        /// The client's version this Dbc is made for
        /// </summary>
        public string ClientVersion { get; set; }

        /// <summary>
        /// The entries stored as Rows[Coloumns]
        /// </summary>
        public List<List<Coloumn>> Entries { get; set; }

        /// <summary>
        /// The Dbc's primary keys
        /// </summary>
        public List<string> PrimaryKeys { get; set; } = new List<string>();

        /// <summary>
        /// The keys referencing foreign Dbcs
        /// </summary>
        public List<ForeignKey> ForeignKeys { get; set; } = new List<ForeignKey>();

        public IEnumerator GetEnumerator()
        {
            foreach (var row in Entries)
                yield return row;
        }

        public List<Coloumn> this[int key]
        {
            get { return Entries[key]; }
        }
    }
}