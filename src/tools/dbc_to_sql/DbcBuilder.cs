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
using System.IO;
using System.Linq;
using System.Xml;

namespace dbc_to_sql
{
    /// <summary>
    /// Builds a Dbc from an XML file describing its format
    /// </summary>
    public class DbcBuilder
    {
        public DbcBuilder(XmlDocument xml)
        {
            xml_ = xml;

            if (xml_ == null || !xml_.HasChildNodes)
                new ArgumentException("The given XmlDocument mustn't be empty!");

            var root = xml_.GetElementsByTagName("file");
            if (root.Count > 1)
                throw new FormatException("The given XmlDocument mustn't contain more than one <file> node!");

            root_ = root[0];

            if (root_.SelectSingleNode("format") == null)
                throw new FormatException("The given XmlDocument must contain a format description!");
        }

        /// <summary>
        /// Build a Dbc from the given input stream
        /// </summary>
        /// <param name="input">A stream containing a dbc file</param>
        /// <returns>The parsed Dbc</returns>
        public Dbc Build(Stream input)
        {
            dbc_ = new Dbc();

            var entries = new List<List<Dbc.Coloumn>>();
            using (var reader = new BinaryReader(input))
            {
                readDbcHead(reader);

                if (root_.SelectSingleNode("format").ChildNodes.Count != head_.coloumnCount)
                {
                    throw new FormatException(
                        string.Format("The given XmlDocument's number of format elements doesn't match the dbc's number of coloumns (given: {0}, expected: {1})",
                            root_.SelectSingleNode("format").ChildNodes.Count,
                            head_.coloumnCount
                        )
                    );
                }

                for (uint row = 0; row < head_.rowCount; ++row)
                {
                    entries.Add(readRow(reader));

                    firstRow_ = false;
                }
            }

            dbc_.Name = root_.SelectSingleNode("name").InnerText;
            dbc_.Entries = entries;

            return dbc_;
        }

        /// <summary>
        /// Reads the Dbc file's header information
        /// </summary>
        /// <param name="reader">The BinaryReader to read from</param>
        private void readDbcHead(BinaryReader reader)
        {
            const int headerSize = 20;

            char[] magic = reader.ReadChars(4);
            if (!magic.SequenceEqual("WDBC"))
                throw new FormatException("File must be a .dbc file!");

            head_.rowCount = reader.ReadUInt32();
            head_.coloumnCount = reader.ReadUInt32();
            head_.coloumnSize = (uint)Math.Ceiling(reader.ReadInt32() / (float)head_.coloumnCount);
            head_.stringBegin = head_.coloumnSize * head_.coloumnCount * head_.rowCount + headerSize;
            head_.stringLength = reader.ReadUInt32();
        }

        /// <summary>
        /// Reads a row from the Dbc
        /// </summary>
        /// <param name="reader">The BinaryReader to read from</param>
        /// <returns>A list of coloumns that were read</returns>
        private List<Dbc.Coloumn> readRow(BinaryReader reader)
        {
            var row = new List<Dbc.Coloumn>();

            var format = root_.SelectSingleNode("format").FirstChild;

            for (uint coloumn = 0; coloumn < head_.coloumnCount; ++coloumn)
            {
                foreach (XmlAttribute attribute in format.Attributes)
                    dumpAttribute(format, attribute);

                switch (format.Name)
                {
                    case "byte": row.Add(readByte(reader, format)); break;
                    case "primary": row.Add(readPrimary(reader, format)); break;
                    case "int": row.Add(readInt(reader, format)); break;
                }

                format = format.NextSibling;
            }

            return row;
        }

        /// <summary>
        /// Reads a byte from the given reader
        /// </summary>
        /// <param name="reader">The BianryReader to read from</param>
        /// <param name="node">The XmlNode describing the current coloumn</param>
        /// <returns>A Dbc.Coloumn describing the stored value</returns>
        private Dbc.Coloumn readByte(BinaryReader reader, XmlNode node)
        {
            var coloumn = new Dbc.Coloumn(reader.ReadByte(), typeof(byte), node.InnerText);
            return coloumn;
        }

        /// <summary>
        /// Reads a uint from the given reader
        /// </summary>
        /// <param name="reader">The BianryReader to read from</param>
        /// <param name="node">The XmlNode describing the current coloumn</param>
        /// <returns>A Dbc.Coloumn describing the stored value</returns>
        private Dbc.Coloumn readUInt(BinaryReader reader, XmlNode node)
        {
            var coloumn = new Dbc.Coloumn(reader.ReadUInt32(), typeof(uint), node.InnerText);
            return coloumn;
        }

        /// <summary>
        /// Reads a primary key from the given reader
        /// </summary>
        /// <param name="reader">The BianryReader to read from</param>
        /// <param name="node">The XmlNode describing the current coloumn</param>
        /// <returns>A Dbc.Coloumn describing the stored value</returns>
        private Dbc.Coloumn readPrimary(BinaryReader reader, XmlNode node)
        {
            var coloumn = readUInt(reader, node);

            if (firstRow_)
                dbc_.PrimaryKeys.Add(coloumn.Name);

            return coloumn;
        }

        /// <summary>
        /// Reads a int from the given reader
        /// </summary>
        /// <param name="reader">The BianryReader to read from</param>
        /// <param name="node">The XmlNode describing the current coloumn</param>
        /// <returns>A Dbc.Coloumn describing the stored value</returns>
        private Dbc.Coloumn readInt(BinaryReader reader, XmlNode node)
        {
            var coloumn = new Dbc.Coloumn(reader.ReadInt32(), typeof(int), node.InnerText);
            return coloumn;
        }

        /// <summary>
        /// Adds a foreign key to the resulting Dbc
        /// </summary>
        /// <param name="node">The node describing the foreign key</param>
        /// <param name="attribute">The XmlAttribute containing the foreign key's data</param>
        private void dumpAttribute(XmlNode node, XmlAttribute attribute)
        {
            switch (attribute.Name)
            {
                case "refersTo": if (firstRow_) dbc_.ForeignKeys.Add(dumpRefersTo(node.InnerText, attribute.InnerText)); break;
            }
        }

        /// <summary>
        /// Returns a Dbc.ForeignKey
        /// </summary>
        /// <param name="coloumnName">The name of the coloumn referring to a foreign column</param>
        /// <param name="text">The unformatted text describing the foreign key's table and coloumn seperated by a period</param>
        private Dbc.ForeignKey dumpRefersTo(string coloumnName, string text)
        {
            var tokens = text.Split('.', 2);
            return new Dbc.ForeignKey(coloumnName, tokens[0], tokens[1]);
        }

        /// <summary>
        /// Describes the header stored within a Dbc file
        /// </summary>
        private struct DbcHead
        {
            /// <summary>
            /// The number of rows stored within the Dbc
            /// </summary>
            public uint rowCount;
            /// <summary>
            /// The number of coloumns stored within the Dbc
            /// </summary>
            public uint coloumnCount;
            /// <summary>
            /// The size of a field within a coloumn
            /// </summary>
            public uint coloumnSize;
            /// <summary>
            /// Points to the beginning of the string block within the Dbc file
            /// </summary>
            public uint stringBegin;
            /// <summary>
            /// The length of the string block
            /// </summary>
            public uint stringLength;
        }

        private DbcHead head_;

        private XmlDocument xml_;
        private XmlNode root_;

        private Dbc dbc_;

        private bool firstRow_ = true;
    }
}