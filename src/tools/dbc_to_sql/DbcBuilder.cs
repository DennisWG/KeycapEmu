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
using System.Text;
using System.Xml;

namespace dbc_to_sql
{
    /// <summary>
    /// Builds a Dbc from an XML file describing its format
    /// </summary>
    public class DbcBuilder
    {
        public DbcBuilder()
        {
        }

        /// <summary>
        /// Build a Dbc from the given input stream
        /// </summary>
        /// <param name="xml">A XmlDocument describing the dbc file's format</param>
        /// <param name="input">A stream containing a dbc file</param>
        /// <returns>The parsed Dbc</returns>
        public Dbc Build(XmlDocument xml, Stream input)
        {
            xml_ = xml;
            dbc_ = new Dbc();

            validateXml();

            dbc_.Name = root_.SelectSingleNode("name").InnerText;
            dbc_.ClientVersion = root_.SelectSingleNode("version").InnerText;

            var entries = new List<List<Dbc.Coloumn>>();
            using (var reader = new BinaryReader(input))
            {
                readDbcHead(reader);

                int expectedColoumns = calculateExpectedDbcSize();

                if (expectedColoumns != head_.coloumnCount)
                {
                    throw new FormatException(
                        string.Format("The given XmlDocument's number of format elements doesn't match the dbc's number of coloumns (given: {0}, expected: {1})",
                            expectedColoumns, head_.coloumnCount)
                    );
                }

                for (uint row = 0; row < head_.rowCount; ++row)
                {
                    entries.Add(readRow(reader));

                    firstRow_ = false;
                }
            }

            dbc_.Entries = entries;

            return dbc_;
        }

        private void validateXml()
        {
            if (xml_ == null || !xml_.HasChildNodes)
                new ArgumentException("The given XmlDocument mustn't be empty!");

            var root = xml_.GetElementsByTagName("file");
            if (root.Count > 1)
                throw new FormatException("The given XmlDocument mustn't contain more than one <file> node!");

            root_ = root[0];

            if (root_.SelectSingleNode("name") == null)
                throw new FormatException("The given XmlDocument must contain a name!");

            if (root_.SelectSingleNode("version") == null)
                throw new FormatException("The given XmlDocument must contain a version!");

            if (root_.SelectSingleNode("format") == null)
                throw new FormatException("The given XmlDocument must contain a format description!");
        }

        private int calculateExpectedDbcSize()
        {
            var root = root_.SelectSingleNode("format");
            int size = 0;

            foreach (XmlNode node in root.ChildNodes)
            {
                switch (node.Name)
                {
                    default: size += 1; break;
                    case "localized_string": size += LocalizedString.SizeForVersion(dbc_.ClientVersion); break;
                }
            }

            return size;
        }

        public DbcHead GetHead(Stream input)
        {
            using (var reader = new BinaryReader(input))
                readDbcHead(reader);

            return head_;
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
            head_.recordSize = reader.ReadUInt32();
            head_.stringBegin = (uint)((float)head_.recordSize / head_.coloumnCount * head_.coloumnCount * head_.rowCount + headerSize);
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

            for (int coloumn = 0; coloumn < head_.coloumnCount;)
            {
                var currentColoumn = new Dbc.Coloumn();

                foreach (XmlAttribute attribute in format.Attributes)
                    dumpAttribute(format, attribute, ref currentColoumn);

                switch (format.Name)
                {
                    default: throw new FormatException(string.Format("Unexpected type '{}'!", format.Name));

                    case "byte": readByte(reader, format, ref currentColoumn); ++coloumn; break;
                    case "primary": readPrimary(reader, format, ref currentColoumn); ++coloumn; break;
                    case "int": readInt(reader, format, ref currentColoumn); ++coloumn; break;
                    case "uint": readUInt(reader, format, ref currentColoumn); ++coloumn; break;
                    case "float": readFloat(reader, format, ref currentColoumn); ++coloumn; break;
                    case "string": readString(reader, format, ref currentColoumn); ++coloumn; break;
                    case "localized_string":
                        readLocalizedString(reader, format, ref currentColoumn);
                        coloumn += LocalizedString.SizeForVersion(dbc_.ClientVersion);
                        break;
                }

                row.Add(currentColoumn);

                format = format.NextSibling;
            }

            return row;
        }

        private bool isNullValue<T>(T value, List<string> nullValues) where T : IComparable
        {
            if (nullValues.Count <= 0)
                return false;

            foreach (string nullStringValue in nullValues)
            {
                var nullValue = (T)Convert.ChangeType(nullStringValue, typeof(T));
                if (value.CompareTo(nullValue) == 0)
                    return true;
            }

            return false;
        }

        /// <summary>
        /// Reads a byte from the given reader
        /// </summary>
        /// <param name="reader">The BianryReader to read from</param>
        /// <param name="node">The XmlNode describing the current coloumn</param>
        /// <returns>A Dbc.Coloumn describing the stored value</returns>
        private void readByte(BinaryReader reader, XmlNode node, ref Dbc.Coloumn currentColoumn)
        {
            var value = reader.ReadByte();
            if (isNullValue(value, currentColoumn.ZeroValues))
                currentColoumn.IsNull = true;

            currentColoumn.Value = value;
            currentColoumn.Type = typeof(byte);
            currentColoumn.Name = node.InnerText;
        }

        /// <summary>
        /// Reads a uint from the given reader
        /// </summary>
        /// <param name="reader">The BianryReader to read from</param>
        /// <param name="node">The XmlNode describing the current coloumn</param>
        /// <returns>A Dbc.Coloumn describing the stored value</returns>
        private void readUInt(BinaryReader reader, XmlNode node, ref Dbc.Coloumn currentColoumn)
        {
            var value = reader.ReadUInt32();
            if (isNullValue(value, currentColoumn.ZeroValues))
                currentColoumn.IsNull = true;

            currentColoumn.Value = value;
            currentColoumn.Type = typeof(uint);
            currentColoumn.Name = node.InnerText;
        }

        /// <summary>
        /// Reads a primary key from the given reader
        /// </summary>
        /// <param name="reader">The BianryReader to read from</param>
        /// <param name="node">The XmlNode describing the current coloumn</param>
        /// <returns>A Dbc.Coloumn describing the stored value</returns>
        private void readPrimary(BinaryReader reader, XmlNode node, ref Dbc.Coloumn currentColoumn)
        {
            readUInt(reader, node, ref currentColoumn);

            if (firstRow_)
                dbc_.PrimaryKeys.Add(currentColoumn.Name);
        }

        /// <summary>
        /// Reads a int from the given reader
        /// </summary>
        /// <param name="reader">The BianryReader to read from</param>
        /// <param name="node">The XmlNode describing the current coloumn</param>
        /// <returns>A Dbc.Coloumn describing the stored value</returns>
        private void readInt(BinaryReader reader, XmlNode node, ref Dbc.Coloumn currentColoumn)
        {
            var value = reader.ReadInt32();
            if (isNullValue(value, currentColoumn.ZeroValues))
                currentColoumn.IsNull = true;

            currentColoumn.Value = value;
            currentColoumn.Type = typeof(int);
            currentColoumn.Name = node.InnerText;
        }

        private void readFloat(BinaryReader reader, XmlNode node, ref Dbc.Coloumn currentColoumn)
        {
            var value = reader.ReadSingle();
            if (isNullValue(value, currentColoumn.ZeroValues))
                currentColoumn.IsNull = true;

            currentColoumn.Value = value;
            currentColoumn.Type = typeof(float);
            currentColoumn.Name = node.InnerText;
        }

        private string escape(string input) => System.Text.RegularExpressions.Regex.Replace(input, @"[\000\010\011\012\015\032\042\047\134\140]", "\\$0");

        private string readStringRef(BinaryReader reader)
        {
            var offset = reader.ReadUInt32();
            var position = reader.BaseStream.Position;

            reader.BaseStream.Position = head_.stringBegin + offset;

            var sb = new StringBuilder();
            char chr = '\0';
            while (reader.BaseStream.Position < head_.stringBegin + head_.stringLength)
            {
                chr = reader.ReadChar();
                if (chr == '\0')
                    break;

                sb.Append(chr);
            }

            reader.BaseStream.Position = position;
            return escape(sb.ToString());
        }

        private void readString(BinaryReader reader, XmlNode node, ref Dbc.Coloumn currentColoumn)
        {
            var value = readStringRef(reader);
            if (isNullValue(value, currentColoumn.ZeroValues))
                currentColoumn.IsNull = true;

            currentColoumn.Value = value;
            currentColoumn.Type = typeof(string);
            currentColoumn.Name = node.InnerText;
        }

        private void readLocalizedString(BinaryReader reader, XmlNode node, ref Dbc.Coloumn currentColoumn)
        {
            var str = new LocalizedString();
            int numLocales = LocalizedString.SizeForVersion(dbc_.ClientVersion) - 1; // substract the flags

            var strings = new List<string>(numLocales);

            for (int i = 0; i < numLocales; ++i)
                strings.Add(readStringRef(reader));

            str.Strings = strings;

            str.Flags = reader.ReadUInt32();

            currentColoumn.Value = str;
            currentColoumn.Type = typeof(LocalizedString);
            currentColoumn.Name = node.InnerText;
        }

        /// <summary>
        /// Adds a foreign key to the resulting Dbc
        /// </summary>
        /// <param name="node">The node describing the foreign key</param>
        /// <param name="attribute">The XmlAttribute containing the foreign key's data</param>
        private void dumpAttribute(XmlNode node, XmlAttribute attribute, ref Dbc.Coloumn coloumn)
        {
            switch (attribute.Name)
            {
                case "refersTo": if (firstRow_) dbc_.ForeignKeys.Add(dumpRefersTo(node.InnerText, attribute.InnerText)); break;
                case "nullValues": coloumn.ZeroValues.AddRange(attribute.InnerText.Split(';')); break;
            }
        }

        /// <summary>
        /// Returns a Dbc.ForeignKey
        /// </summary>
        /// <param name="coloumnName">The name of the coloumn referring to a foreign column</param>
        /// <param name="text">The unformatted text describing the foreign key's table and coloumn seperated by a period</param>
        private Dbc.ForeignKey dumpRefersTo(string coloumnName, string text)
        {
            if (!text.Contains("."))
                throw new FormatException("Foreign key must have format \"Table.Coloumn\"!");

            var tokens = text.Split('.', 2);
            return new Dbc.ForeignKey(coloumnName, tokens[0], tokens[1]);
        }

        /// <summary>
        /// Describes the header stored within a Dbc file
        /// </summary>
        public struct DbcHead
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
            /// The size of an record (the whole row)
            /// </summary>
            public uint recordSize;
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