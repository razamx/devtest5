import sys
import os
import argparse


class ArtifactsReportGenerator:
    def __init__(self):
        self.args = ArtifactsReportGenerator.get_parser().parse_args(sys.argv[1:])
        self.curdir = os.path.abspath(os.path.dirname(sys.argv[0]))
        self.page_template = """
<script>
function copySelectedText(copy_result_span_id){{
    span = document.getElementById(copy_result_span_id);
    try {{
        document.execCommand("copy")
        span.textContent="Successfully copied to clipboard.";
        span.style.color = "black";
    }}
    catch(e) {{
        span.textContent="Copying failed! Please copy the path manually.";
        span.style.color = "red";
    }}
}}
function selectSpanText(id){{
    var field = document.getElementById(id)
    var range = document.createRange();
    range.selectNodeContents(field);
    var selection = window.getSelection();
    selection.removeAllRanges();
    selection.addRange(range);
}}
</script>

<body>
<div>
    <b>URL to server:</b> <br/>
    <input type="button" value="Copy to clipboard" onclick="selectSpanText('url');copySelectedText('copy_result');">
    <span style="color: blue; font-weight: 500;" id="url"><a href="{URL}">{URL}</a></span>
    <br/><br/>
    <span id="copy_result"></span>
</div>
</body>"""

    @staticmethod
    def get_parser():
        parser = argparse.ArgumentParser()
        parser.add_argument('--force', action='store_true', help="Overwrite file if already exists")
        parser.add_argument('--url', required=True, help="URL to artifacts on server to embed into the report")
        parser.add_argument('--artifact-path', required=True, help="A target path to generated report to be registed as build Artifact in TeamCity")
        parser.add_argument('--output-file-path', required=True, help="A target report file path including name. Use relative path to WorkDir")
        return parser

    def generate_report(self):
        if os.path.exists(self.args.output_file_path) and not self.args.force:
            raise RuntimeError("File " + self.args.output_file_path +
                               " already exists. Please use --force to overwrite existing files")
        target_dir = os.path.dirname(os.path.abspath(self.args.output_file_path))
        if not os.path.exists(target_dir):
            os.makedirs(target_dir)
        with open(self.args.output_file_path, "w") as f:
            f.write(self.page_template.format(URL=self.args.url))
        print("successfully generated report: " + os.path.abspath(self.args.output_file_path))

    def register_report_as_artifact(self):
        artifact_rule = self.args.output_file_path + " => " + self.args.artifact_path
        print("##teamcity[publishArtifacts '{ARTIFACT_RULE}']".format(ARTIFACT_RULE=artifact_rule))

if __name__ == "__main__":
    generator = ArtifactsReportGenerator()
    generator.generate_report()
    generator.register_report_as_artifact()
