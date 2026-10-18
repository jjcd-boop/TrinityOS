#!/usr/bin/env python3
"""H3C3: survey Trinity's tree builder against official html5lib tree tests.

The test corpus is loaded from a PINNED historical revision of
html5lib/html5lib-tests. Cases are used verbatim as inputs, with html5lib 1.1
as the *element/text tree-shape* oracle. The .dat #document expectations are
not yet checked, nor are namespaces, attrs, comments, parse errors, etc.
No Python or external parser code ships with Trinity OS.
"""
import argparse
import collections
import pathlib
import subprocess
import sys

import html5lib

DATASET_FILES = (
    "adoption01.dat",
    "adoption02.dat",
    "blocks.dat",
    "tables01.dat",
    "inbody01.dat",
    "tests1.dat",
    "tests2.dat",
    "tests3.dat",
)

def fixture_cases(text):
    """Parse #data blocks, retaining input newlines except terminal LF."""
    chunks = text.replace("\r\n", "\n").split("#data\n")
    for chunk in chunks[1:]:
        # #data section can include literal '#' markers; we stop at the first
        # section separator line, not at a substring elsewhere in HTML.
        data, separator, _ = chunk.partition("\n#errors\n")
        if not separator:
            # The current official tests always carry #errors. A malformed
            # fixture is not a skip: surface it as an ingestion error.
            raise ValueError("Missing #errors separator in official corpus")
        yield data, chunk

def expected_body(html):
    root = html5lib.parse(html, treebuilder="etree", namespaceHTMLElements=False)
    body = root.find("body")
    if body is None:
        return None  # frameset documents and other non-body modes
    output = []
    def escape(s):
        return (s.replace("\\", "\\\\").replace("\n", "\\n")
                 .replace("\r", "\\r").replace("\t", "\\t"))
    def append_text(s):
        if s:
            line = "T:" + escape(s)
            # The current Trinity dumper splits long text at 255 bytes.
            # Corpus matching is exploratory until a structured DOM stream
            # with canonical text coalescing is implemented.
            output.append(line)
    def visit(el):
        append_text(el.text)
        for child in el:
            if isinstance(child.tag, str):
                output.append("E:" + child.tag)
                visit(child)
                output.append("/E")
            append_text(child.tail)
    visit(body)
    return output

def main():
    cli=argparse.ArgumentParser()
    cli.add_argument("executable",type=pathlib.Path)
    cli.add_argument("dataset",type=pathlib.Path)
    cli.add_argument("--maximum",type=int,default=240,
                     help="maximum examined corpus inputs across all files")
    cli.add_argument("--minimum-scanned",type=int,default=120)
    cli.add_argument("--report",type=pathlib.Path)
    args=cli.parse_args()
    if args.maximum<1 or args.minimum_scanned<1:
        cli.error("must examine at least one input")
    counts=collections.Counter()
    sample_mismatches=[]
    per_file={}
    for basename in DATASET_FILES:
        if counts["examined"]>=args.maximum:break
        path=args.dataset / basename
        if not path.is_file():
            raise FileNotFoundError(path)
        totals=collections.Counter()
        for case_id,(html, full_chunk) in enumerate(
                fixture_cases(path.read_text(encoding="utf-8-sig")), start=1):
            if counts["examined"]>=args.maximum:break
            counts["examined"]+=1
            totals["examined"]+=1
            reason=None
            if "#document-fragment\n" in full_chunk:
                reason="fragment_mode_not_implemented"
            elif "#script-on\n" in full_chunk:
                reason="scripting_mode_not_implemented"
            elif "\x00" in html:
                reason="nul_input_not_supported_by_host_dumper"
            elif len(html.encode("utf-8")) > 16000:
                reason="host_input_budget"
            if reason is not None:
                counts["skip_"+reason]+=1
                totals["skip_"+reason]+=1
                continue
            try:
                result=subprocess.run([str(args.executable), html],
                     capture_output=True,encoding="utf-8",errors="replace",
                     check=False,timeout=3)
                if result.returncode:
                    counts["parser_rejected"]+=1
                    totals["parser_rejected"]+=1
                    continue
                actual=result.stdout.splitlines()
                expected=expected_body(html)
                if expected is None:
                    counts["skip_nonbody_reference"]+=1
                    totals["skip_nonbody_reference"]+=1
                    continue
                if actual == expected:
                    counts["tree_match"]+=1
                    totals["tree_match"]+=1
                else:
                    counts["tree_mismatch"]+=1
                    totals["tree_mismatch"]+=1
                    if len(sample_mismatches)<25:
                        sample_mismatches.append({
                            "fixture":basename,"case":case_id,
                            "input":html[:200],"expected":expected[:30],
                            "trinity":actual[:30]
                        })
            except (subprocess.TimeoutExpired,UnicodeError,ValueError) as err:
                counts["execution_error"]+=1
                totals["execution_error"]+=1
                if len(sample_mismatches)<25:
                    sample_mismatches.append({
                        "fixture":basename,"case":case_id,
                        "error":str(err)[:200]
                    })
        per_file[basename]=dict(totals)
    print("H3C3 OFFICIAL CORPUS SURVEY (not HTML5 conformance)")
    for key in sorted(counts):
        print(f"  {key}: {counts[key]}")
    for name,values in per_file.items():
        print(f"  {name}: {values}")
    print(f"  first_differences: {sample_mismatches[:6]}")
    # Explicitly keep this exploratory. We fail on corpus ingestion or
    # not enough tests, not on mismatches from an incomplete parser.
    passed = counts["examined"]>=args.minimum_scanned and (
        counts["tree_match"]+counts["tree_mismatch"]+
        counts["parser_rejected"]+counts["execution_error"]+
        sum(value for key,value in counts.items() if key.startswith("skip_"))
    ) == counts["examined"]
    if args.report:
        import json
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps({
            "dataset": "html5lib-tests pinned pre-June-2026 removal",
            "comparison": "body element/text tree shape via html5lib 1.1",
            "passed_collection":passed,
            "counts":dict(counts),
            "per_file":per_file,
            "samples":sample_mismatches
        },indent=2)+"\n",encoding="utf-8")
    return 0 if passed else 1

if __name__=="__main__":
    sys.exit(main())
