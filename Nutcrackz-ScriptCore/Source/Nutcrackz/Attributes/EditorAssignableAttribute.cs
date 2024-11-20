using System;

namespace Nutcrackz
{
    [AttributeUsage(AttributeTargets.Class, AllowMultiple = false, Inherited = true)]
    internal class EditorAssignableAttribute : Attribute { }
}