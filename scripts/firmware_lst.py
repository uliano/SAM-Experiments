from pathlib import Path
import html
import re
import subprocess

from SCons.Script import Action

Import("env")


ASM_LINE_RE = re.compile(r"^(\s*)([0-9a-fA-F]+):\s+((?:[0-9a-fA-F]{2,8}\s+)+)(.*)$")
REGISTER_RE = re.compile(r"\b(r1[0-5]|r[0-9]|sp|lr|pc|xpsr|apsr|ipsr|epsr)\b")
IMMEDIATE_RE = re.compile(r"#[A-Za-z0-9_+\-xXa-fA-F]+")
MNEMONIC_RE = re.compile(r"^(\s*)([A-Za-z][A-Za-z0-9_.]*)(.*)$")
ARM_MNEMONICS = {
    "adc",
    "adcs",
    "add",
    "adds",
    "adr",
    "and",
    "ands",
    "asr",
    "asrs",
    "b",
    "bcc",
    "bcs",
    "beq",
    "bge",
    "bgt",
    "bhi",
    "bkpt",
    "bl",
    "ble",
    "blo",
    "bls",
    "blt",
    "bmi",
    "bne",
    "bpl",
    "bx",
    "bvc",
    "bvs",
    "bic",
    "bics",
    "cmn",
    "cmp",
    "cpsid",
    "cpsie",
    "dmb",
    "dsb",
    "eor",
    "eors",
    "isb",
    "ldm",
    "ldmia",
    "ldr",
    "ldrb",
    "ldrh",
    "ldrsb",
    "ldrsh",
    "lsl",
    "lsls",
    "lsr",
    "lsrs",
    "mov",
    "movs",
    "mrs",
    "msr",
    "mul",
    "muls",
    "mvn",
    "mvns",
    "nop",
    "orr",
    "orrs",
    "pop",
    "push",
    "rev",
    "rev16",
    "revsh",
    "ror",
    "rors",
    "sbc",
    "sbcs",
    "sev",
    "stm",
    "stmia",
    "str",
    "strb",
    "strh",
    "sub",
    "subs",
    "svc",
    "sxtb",
    "sxth",
    "tst",
    "uxtb",
    "uxth",
    "wfe",
    "wfi",
    "yield",
}


def _tool_path(name):
    value = env.subst(name).strip('"')
    if value and value != name:
        return value

    cc = env.subst("$CC").strip('"')
    for suffix in ("gcc", "g++"):
        if cc.endswith(suffix):
            candidate = cc[: -len(suffix)] + "objdump"
            if Path(candidate).exists():
                return candidate

    found = env.WhereIs("arm-none-eabi-objdump")
    if found:
        return found

    return "arm-none-eabi-objdump"


def _highlight_operands(text):
    escaped = html.escape(text)
    escaped = REGISTER_RE.sub(r'<span class="reg">\1</span>', escaped)
    escaped = IMMEDIATE_RE.sub(lambda match: f'<span class="imm">{match.group(0)}</span>', escaped)
    return escaped


def _highlight_asm_tail(text):
    comment = ""
    body = text
    if "@" in text:
        body, _, comment_text = text.partition("@")
        comment = '<span class="comment">@' + html.escape(comment_text) + "</span>"

    match = MNEMONIC_RE.match(body.rstrip())
    if not match:
        return _highlight_operands(body) + comment

    leading, mnemonic, operands = match.groups()
    return (
        html.escape(leading)
        + f'<span class="mnemonic">{html.escape(mnemonic)}</span>'
        + _highlight_operands(operands)
        + comment
    )


def _highlight_listing_line(line):
    match = ASM_LINE_RE.match(line)
    if match:
        indent, address, data, tail = match.groups()
        mnemonic = MNEMONIC_RE.match(tail.rstrip())
        if not mnemonic or mnemonic.group(2).split(".", 1)[0].lower() not in ARM_MNEMONICS:
            return (
                html.escape(indent)
                + f'<span class="addr">{html.escape(address)}:</span>'
                + "   "
                + f'<span class="bytes">{html.escape(data.rstrip())}</span>'
                + html.escape(tail)
            )

        return (
            '<span class="asm">'
            + html.escape(indent)
            + f'<span class="addr">{html.escape(address)}:</span>'
            + "   "
            + f'<span class="bytes">{html.escape(data.rstrip())}</span>'
            + "   "
            + _highlight_asm_tail(tail)
            + "</span>"
        )

    if ".cpp:" in line or ".hpp:" in line or ".c:" in line or ".h:" in line:
        return f'<span class="loc">{html.escape(line)}</span>'

    if line.endswith(":"):
        return f'<span class="label">{html.escape(line)}</span>'

    return f'<span class="src">{html.escape(line)}</span>'


def _write_html_listing(lst, text):
    path = lst.with_suffix(".html")
    body = "\n".join(_highlight_listing_line(line) for line in text.splitlines())
    path.write_text(
        """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>firmware.lst</title>
<style>
:root {
  color-scheme: dark;
  --bg: #111318;
  --fg: #d7dde8;
  --muted: #7e8796;
  --addr: #6ab7ff;
  --bytes: #9aa7b9;
  --mnemonic: #f2c86b;
  --reg: #8ed7a7;
  --imm: #d8a8ff;
  --comment: #7fdaff;
  --loc: #ff9f7a;
  --label: #c7d4ff;
}
body {
  margin: 0;
  background: var(--bg);
  color: var(--fg);
}
pre {
  box-sizing: border-box;
  margin: 0;
  min-height: 100vh;
  padding: 20px 24px;
  font: 13px/1.45 Consolas, "Cascadia Mono", "Courier New", monospace;
  tab-size: 8;
  white-space: pre;
}
.addr { color: var(--addr); }
.bytes { color: var(--bytes); }
.mnemonic { color: var(--mnemonic); font-weight: 600; }
.reg { color: var(--reg); }
.imm { color: var(--imm); }
.comment { color: var(--comment); }
.loc { color: var(--loc); }
.label { color: var(--label); font-weight: 600; }
.src { color: var(--fg); }
</style>
</head>
<body>
<pre>"""
        + body
        + """</pre>
</body>
</html>
""",
        encoding="utf-8",
    )


def _emit_listing(source, target, env):
    elf = Path(env.subst("$BUILD_DIR/${PROGNAME}.elf"))
    lst = elf.with_suffix(".lst")
    objdump = _tool_path("$OBJDUMP")

    completed = subprocess.run(
        [objdump, "-d", "-S", "-C", "-l", "--wide", str(elf)],
        stdout=subprocess.PIPE,
        check=True,
    )

    listing = completed.stdout.decode("utf-8", errors="replace").expandtabs(8)
    lst.parent.mkdir(parents=True, exist_ok=True)
    lst.write_text(listing, encoding="utf-8")
    _write_html_listing(lst, listing)
    return 0


env.AddPostAction(
    "$BUILD_DIR/${PROGNAME}.elf",
    Action(_emit_listing, "Building $BUILD_DIR/${PROGNAME}.lst and .html"),
)
