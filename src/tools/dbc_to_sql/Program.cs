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
using System.IO;
using System.Xml;

namespace dbc_to_sql
{
    class Program
    {
        static void PrintHelp()
        {
            Console.WriteLine("{0}", System.AppDomain.CurrentDomain.FriendlyName);
            Console.WriteLine("Converts a .dbc file to a .sql file using a .xml file describing the dbc format");
            Console.WriteLine();
            Console.WriteLine("Arguments:");

            Console.WriteLine("-i\tPath to the .dbc file");
            Console.WriteLine("-x\tPath to the .xml file describing the .dbc file");
            Console.WriteLine("-o\tPath to the .sql file that will be created");
            Console.WriteLine("-s\tShows the syntax supported for the .xml file");
            Console.WriteLine();

            Console.WriteLine("Example:");
            Console.WriteLine("{0} -i ChrClasses.dbc -x ChrClasses.xml -o ChrClasses.sql", System.AppDomain.CurrentDomain.FriendlyName);
        }

        static void PrintSyntax()
        {
            Console.WriteLine("{0} syntax description", System.AppDomain.CurrentDomain.FriendlyName);
            Console.WriteLine();

            Console.WriteLine("Basic file format:");
            Console.WriteLine("<?xml version=\"1.0\"?>\n<file>\n    <name>YOUR DBC NAME HERE</name>\n    <format>\n        COLOUMN DESCRIPTION HERE\n    </format>\n</file>");
            Console.WriteLine();

            Console.WriteLine("COLOUMN description elements:");
            Console.WriteLine("<TYPE OPTIONAL_ATTRIBUTES>COLOUMN NAME</TYPE>");
            Console.WriteLine();

            Console.WriteLine("Types:");
            Console.WriteLine("Name\t| SQL type\t\t\t| Attributes");
            Console.WriteLine("--------+-------------------------------+------------");
            Console.WriteLine("primary\t| INT UNSIGNED PRIMARY KEY\t|");
            Console.WriteLine("int\t| INT\t\t\t\t| refersTo");
            Console.WriteLine("byte\t| TINYINT\t\t\t| refersTo");
            Console.WriteLine();

            Console.WriteLine("Attributes:");
            Console.WriteLine("Name\t\t| Description\t\t\t\t\t\t\t| Example");
            Console.WriteLine("----------------+---------------------------------------------------------------+--------------------------");
            Console.WriteLine("refersTo\t| Creates a FOREIGN KEY with CASCADE default constraints\t| refersTo=\"Table.Coloumn\"");
        }

        class Args
        {
            public string InputPath { get; private set; }
            public string XmlPath { get; private set; }
            public string OutputPath { get; private set; }

            public static Args build(string[] arguments)
            {
                Args args = new Args();

                args.extractOne(arguments[0], arguments[1]);
                args.extractOne(arguments[2], arguments[3]);
                args.extractOne(arguments[4], arguments[5]);

                return args;
            }

            private void extractOne(string key, string value)
            {
                switch (key)
                {
                    default: throw new ArgumentException("Unknown argument: " + key);
                    case "-i": InputPath = value; break;
                    case "-x": XmlPath = value; break;
                    case "-o": OutputPath = value; break;
                }
            }
        }

        static void Main(string[] args)
        {
            if (args.Length == 1 && args[0] == "-s")
            {
                PrintSyntax();
                return;
            }
            if (args.Length != 6)
            {
                PrintHelp();
                return;
            }

            Args arguments;
            try
            {
                arguments = Args.build(args);
            }
            catch (ArgumentException e)
            {
                Console.WriteLine(e.Message);
                Console.WriteLine("Write '{0}' without arguments to get a help prompt", System.AppDomain.CurrentDomain.FriendlyName);
                return;
            }

            var xml = new XmlDocument();
            xml.Load(arguments.XmlPath);

            DbcBuilder builder = new DbcBuilder(xml);
            var dbc = builder.Build(File.Open(arguments.InputPath, FileMode.Open, FileAccess.Read));

            var sqlBuilder = new SqlBuilder();
            var sql = sqlBuilder.Build(dbc);

            Directory.CreateDirectory(Path.GetDirectoryName(arguments.OutputPath));
            using (var writer = new StreamWriter(File.Open(arguments.OutputPath, FileMode.Create)))
                writer.Write(sql);
        }
    }
}
