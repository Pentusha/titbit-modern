// Bone serialisation to the engine's XML text format (0x40ced0, 0x40d370).
extern "C" __declspec(dllimport) int __stdcall lstrlenA(const char *);

// The engine string, viewed through the out-of-line members already published
// in src/native (their mangled names bind to the original addresses).
class ObservedString {
public:
 char *data;
 ObservedString();
 ObservedString(const char *text);                       // 0x409400
 ObservedString(const ObservedString &other);            // 0x402040
 ~ObservedString();                                      // 0x402300
 ObservedString &Assign(const char *text);               // 0x403d90
 // Refcounted assignment (0x4023e0). Without it the implicit operator= copies
 // the pointer only and `name = Concat(...)` freed name's buffer (use-after-free).
 ObservedString &operator=(const ObservedString &other);
 int GetLength() const { return ((int *)data)[-2]; }
 void Append_004075a0(int length, const char *text);
 ObservedString &Append_00409960(const char *text);
 ObservedString &AppendString_0040e1d0(const ObservedString &other);
 void AppendText(const char *text) { Append_004075a0(lstrlenA(text), text); }
};
inline ObservedString::ObservedString() { extern char *observed_empty_string; data = observed_empty_string; }
void __cdecl FormatArchiveString(ObservedString *out, const char *format, ...);   // 0x402630
ObservedString __stdcall Concat_0040e070(const char *a, const ObservedString &b);

struct BoneVec { float x, y, z; };

// "<name x="..." y="..." z="..." />\n\r"
ObservedString __stdcall WriteVec3_0040d370(const BoneVec *v, ObservedString name) {
 ObservedString number;
 name = Concat_0040e070("<", name);
 name.AppendText(" ");
 name.AppendText("x=\"");
 FormatArchiveString(&number, "%.4f", (double)v->x);
 name.Append_004075a0(number.GetLength(), number.data);
 name.AppendText("\" ");
 name.AppendText("y=\"");
 FormatArchiveString(&number, "%.4f", (double)v->y);
 name.Append_004075a0(number.GetLength(), number.data);
 name.AppendText("\" ");
 name.AppendText("z=\"");
 FormatArchiveString(&number, "%.4f", (double)v->z);
 name.Append_004075a0(number.GetLength(), number.data);
 name.AppendText("\" ");
 name.AppendText("/>");
 name.AppendText("\n\r");
 return name;
}

struct BoneData {
 int index;                 // +0
 BoneVec position;          // +4
 float rotation[4];         // +0x10
 BoneVec scale;             // +0x20
 BoneVec center;            // +0x2c
 BoneVec bbox;              // +0x38
 BoneVec extents;           // +0x44
 float radius;              // +0x50
 float local[16], world[16];
 int parent;                // +0xd4
 BoneVec GetRotation_0040dc40() const;
 ObservedString Export_0040ced0() const;
};

ObservedString BoneData::Export_0040ced0() const {
 ObservedString xml, number;
 xml.AppendText("<bone ");
 xml.AppendText("bone_id=\"");
 FormatArchiveString(&number, "%i", index);
 xml.Append_004075a0(number.GetLength(), number.data);
 xml.AppendText("\" ");
 if (parent != -1) {
  xml.AppendText("parent_bone_id=\"");
  FormatArchiveString(&number, "%i", parent);
  xml.Append_004075a0(number.GetLength(), number.data);
  xml.AppendText("\" ");
 }
 xml.AppendText("radius=\"");
 FormatArchiveString(&number, "%.4f", (double)radius);
 xml.Append_004075a0(number.GetLength(), number.data);
 xml.AppendText("\" ");
 xml.AppendText(">");
 xml.AppendText("\n\r");
 { ObservedString s = WriteVec3_0040d370(&position, ObservedString("position")); xml.Append_004075a0(s.GetLength(), s.data); }
 { ObservedString s = WriteVec3_0040d370(&center, ObservedString("center")); xml.Append_004075a0(s.GetLength(), s.data); }
 { ObservedString s = WriteVec3_0040d370(&bbox, ObservedString("bbox")); xml.Append_004075a0(s.GetLength(), s.data); }
 BoneVec angles = GetRotation_0040dc40();
 xml.AppendString_0040e1d0(WriteVec3_0040d370(&angles, ObservedString("rotation")));
 xml.AppendString_0040e1d0(WriteVec3_0040d370(&scale, ObservedString("scale")));
 xml.Append_00409960("</bone>");
 xml.Append_00409960("\n\r");
 return xml;
}
