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

using System.Collections.Generic;
using System.Text;

namespace dbc_to_sql
{
    public class LocalizedString
    {
        public List<string> Strings { get; set; } = new List<string>();
        public uint Flags { get; set; } = 0;

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            bool first = true;
            foreach (string str in Strings)
            {
                if (first)
                    sb.AppendFormat("{0}'", str);
                else
                    sb.AppendFormat(", '{0}'", str);
                first = false;
            }

            sb.AppendFormat(", '{0}", Flags);

            return sb.ToString();
        }

        public static int SizeForVersion(string version)
        {
            switch (version)
            {
                default: return 0;
                case "2.4.3": return 16 + 1;
            }
        }
    }
}