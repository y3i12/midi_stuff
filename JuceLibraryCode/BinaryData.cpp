/* ==================================== JUCER_BINARY_RESOURCE ====================================

   This is an auto-generated file: Any edits you make may be overwritten!

*/

#include <cstring>

namespace BinaryData
{

//================== midi_stuff.xml ==================
static const unsigned char temp_binary_data_0[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
"\r\n"
"<magic>\r\n"
"  <Styles>\r\n"
"    <Style name=\"default\">\r\n"
"      <Nodes/>\r\n"
"      <Classes>\r\n"
"        <plot-view border=\"2\" background-color=\"black\" border-color=\"silver\" display=\"contents\"/>\r\n"
"        <nomargin margin=\"0\" padding=\"0\" border=\"0\"/>\r\n"
"        <group margin=\"5\" padding=\"5\" border=\"2\" flex-direction=\"column\"/>\r\n"
"        <transparent background-color=\"transparentblack\"/>\r\n"
"      </Classes>\r\n"
"      <Types>\r\n"
"        <Slider border=\"0\" slider-textbox=\"textbox-below\"/>\r\n"
"        <ToggleButton border=\"0\" max-height=\"50\" caption-size=\"0\" text=\"Active\"/>\r\n"
"        <TextButton border=\"0\" max-height=\"50\" caption-size=\"0\"/>\r\n"
"        <ComboBox border=\"0\" max-height=\"50\" caption-size=\"0\"/>\r\n"
"        <Plot border=\"0\" margin=\"0\" padding=\"0\" background-color=\"00000000\"\r\n"
"              radius=\"0\"/>\r\n"
"        <XYDragComponent border=\"0\" margin=\"0\" padding=\"0\" background-color=\"00000000\"\r\n"
"                         radius=\"0\"/>\r\n"
"      </Types>\r\n"
"      <Palettes>\r\n"
"        <default/>\r\n"
"      </Palettes>\r\n"
"    </Style>\r\n"
"  </Styles>\r\n"
"  <View id=\"root\" resizable=\"1\" resize-corner=\"1\">\r\n"
"    <View class=\"parameters nomargin\">\r\n"
"      <Slider caption=\"Lowest\" parameter=\"lowest\" slider-type=\"linear-vertical\"\r\n"
"              interval=\"1\" max-value=\"12\" min-value=\"0\"/>\r\n"
"      <Slider caption=\"Highest\" parameter=\"highest\" slider-type=\"linear-vertical\"\r\n"
"              interval=\"1\" min-value=\"0\" max-value=\"12\"/>\r\n"
"      <Slider caption=\"Lowest 2\" parameter=\"lowest_2\" slider-type=\"linear-vertical\"\r\n"
"              min-value=\"0\" max-value=\"12\" interval=\"1\"/>\r\n"
"    </View>\r\n"
"  </View>\r\n"
"</magic>\r\n";

const char* midi_stuff_xml = (const char*) temp_binary_data_0;

//================== LICENSE ==================
static const unsigned char temp_binary_data_1[] =
"This is free and unencumbered software released into the public domain.\r\n"
"\r\n"
"Anyone is free to copy, modify, publish, use, compile, sell, or\r\n"
"distribute this software, either in source code form or as a compiled\r\n"
"binary, for any purpose, commercial or non-commercial, and by any\r\n"
"means.\r\n"
"\r\n"
"In jurisdictions that recognize copyright laws, the author or authors\r\n"
"of this software dedicate any and all copyright interest in the\r\n"
"software to the public domain. We make this dedication for the benefit\r\n"
"of the public at large and to the detriment of our heirs and\r\n"
"successors. We intend this dedication to be an overt act of\r\n"
"relinquishment in perpetuity of all present and future rights to this\r\n"
"software under copyright law.\r\n"
"\r\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\r\n"
"EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF\r\n"
"MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.\r\n"
"IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR\r\n"
"OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,\r\n"
"ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\r\n"
"OTHER DEALINGS IN THE SOFTWARE.\r\n"
"\r\n"
"For more information, please refer to <https://unlicense.org>\r\n";

const char* LICENSE = (const char*) temp_binary_data_1;

//================== README.md ==================
static const unsigned char temp_binary_data_2[] =
"# midi_stuff\n"
" midi processing stuff vst\n";

const char* README_md = (const char*) temp_binary_data_2;


const char* getNamedResource (const char* resourceNameUTF8, int& numBytes);
const char* getNamedResource (const char* resourceNameUTF8, int& numBytes)
{
    unsigned int hash = 0;

    if (resourceNameUTF8 != nullptr)
        while (*resourceNameUTF8 != 0)
            hash = 31 * hash + (unsigned int) *resourceNameUTF8++;

    switch (hash)
    {
        case 0x47775bae:  numBytes = 1607; return midi_stuff_xml;
        case 0x34bc1021:  numBytes = 1235; return LICENSE;
        case 0x64791dc8:  numBytes = 40; return README_md;
        default: break;
    }

    numBytes = 0;
    return nullptr;
}

const char* namedResourceList[] =
{
    "midi_stuff_xml",
    "LICENSE",
    "README_md"
};

const char* originalFilenames[] =
{
    "midi_stuff.xml",
    "LICENSE",
    "README.md"
};

const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8);
const char* getNamedResourceOriginalFilename (const char* resourceNameUTF8)
{
    for (unsigned int i = 0; i < (sizeof (namedResourceList) / sizeof (namedResourceList[0])); ++i)
        if (strcmp (namedResourceList[i], resourceNameUTF8) == 0)
            return originalFilenames[i];

    return nullptr;
}

}
