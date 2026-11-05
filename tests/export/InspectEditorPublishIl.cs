#:property PublishAot=false
#:property SelfContained=false
using System.Reflection;
using System.Reflection.Emit;
using System.Reflection.Metadata;
using System.Reflection.Metadata.Ecma335;
using System.Reflection.PortableExecutable;
using System.Text.Json;

if (args.Length != 1) return 2;
var opcodes = typeof(OpCodes).GetFields(BindingFlags.Public | BindingFlags.Static)
    .Where(field => field.FieldType == typeof(OpCode))
    .Select(field => (OpCode)field.GetValue(null)!)
    .ToDictionary(op => unchecked((ushort)op.Value));
using var file = File.OpenRead(args[0]);
using var pe = new PEReader(file);
var metadata = pe.GetMetadataReader();
var locations = new List<object>();
var unexpected = false;
foreach (var typeHandle in metadata.TypeDefinitions)
{
    var type = metadata.GetTypeDefinition(typeHandle);
    foreach (var handle in type.GetMethods())
    {
        var method = metadata.GetMethodDefinition(handle);
        if (method.RelativeVirtualAddress == 0) continue;
        var il = pe.GetMethodBody(method.RelativeVirtualAddress).GetILBytes()!;
        for (var offset = 0; offset < il.Length;)
        {
            var instruction = offset;
            ushort value = il[offset++];
            if (value == 0xfe) value = (ushort)(0xfe00 | il[offset++]);
            var op = opcodes[value];
            if (op.Value == OpCodes.Ldstr.Value)
            {
                var token = BitConverter.ToInt32(il, offset);
                var literal = metadata.GetUserString(MetadataTokens.UserStringHandle(token & 0x00ffffff));
                if (literal == "export")
                {
                    var owner = metadata.GetString(type.Namespace) + "." + metadata.GetString(type.Name);
                    var methodName = metadata.GetString(method.Name);
                    locations.Add(new { owner, method = methodName, offset = instruction, literal });
                    unexpected |= owner != "PixieEditor.Native.ProjectBuild" || methodName != "Cache";
                }
            }
            offset += op.OperandType switch
            {
                OperandType.InlineNone => 0,
                OperandType.ShortInlineBrTarget or OperandType.ShortInlineI or OperandType.ShortInlineVar => 1,
                OperandType.InlineVar => 2,
                OperandType.InlineBrTarget or OperandType.InlineField or OperandType.InlineI or OperandType.InlineMethod or
                OperandType.InlineSig or OperandType.InlineString or OperandType.InlineTok or OperandType.InlineType or
                OperandType.ShortInlineR => 4,
                OperandType.InlineI8 or OperandType.InlineR => 8,
                OperandType.InlineSwitch => checked(4 + 4 * BitConverter.ToInt32(il, offset)),
                _ => throw new InvalidDataException($"Unhandled IL operand {op.OperandType}.")
            };
        }
    }
}
Console.WriteLine(JsonSerializer.Serialize(locations, new JsonSerializerOptions { WriteIndented = true }));
return unexpected ? 1 : 0;
