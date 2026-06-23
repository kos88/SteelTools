import sys
from pathlib import Path

try:
    from PySide6.QtWidgets import QApplication, QVBoxLayout, QWidget
    from PySide6.QtWebEngineWidgets import QWebEngineView
    from PySide6.QtCore import QUrl
    from PySide6.QtGui import QTextDocument
except ImportError:
    from PySide2.QtWidgets import QApplication, QVBoxLayout, QWidget
    from PySide2.QtWebEngineWidgets import QWebEngineView
    from PySide2.QtCore import QUrl
    from PySide2.QtGui import QTextDocument


class MarkdownViewer(QWidget):
    def __init__(self, md_file):
        super().__init__()
        self.setWindowTitle("Markdown Viewer")
        self.resize(800, 600)

        md_path = Path(md_file).resolve()

        with open(md_path, 'r', encoding='utf-8') as f:
            md_content = f.read()

        # Use QTextDocument to convert markdown to HTML
        doc = QTextDocument()
        doc.setMarkdown(md_content)
        html_body = doc.toHtml()

        # For videos and other HTML that QTextDocument might strip,
        # extract raw HTML blocks from original markdown
        import re
        video_pattern = r'<video.*?</video>'
        iframe_pattern = r'<iframe.*?</iframe>'

        videos = re.findall(video_pattern, md_content, re.DOTALL)
        iframes = re.findall(iframe_pattern, md_content, re.DOTALL)

        # Create complete HTML
        html = f"""
        <!DOCTYPE html>
        <html>
        <head>
            <meta charset="UTF-8">
            <base href="file:///{str(md_path.parent).replace(chr(92), '/')}/">
            <style>
                body {{ 
                    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Arial, sans-serif;
                    padding: 20px; 
                    max-width: 900px;
                    margin: 0 auto;
                    line-height: 1.6;
                }}
                img {{ max-width: 100%; height: auto; }}
                video {{ max-width: 100%; }}
                iframe {{ max-width: 100%; }}
                pre {{ background: #f4f4f4; padding: 15px; border-radius: 5px; overflow-x: auto; }}
                code {{ background: #f4f4f4; padding: 2px 5px; border-radius: 3px; }}
                pre code {{ background: none; padding: 0; }}
            </style>
        </head>
        <body>
            {html_body}
            {" ".join(videos + iframes)}
        </body>
        </html>
        """

        web_view = QWebEngineView()
        web_view.setHtml(html, QUrl.fromLocalFile(str(md_path.parent)))

        layout = QVBoxLayout()
        layout.setContentsMargins(0, 0, 0, 0)
        layout.addWidget(web_view)
        self.setLayout(layout)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    # Use the full path to your markdown file
    viewer = MarkdownViewer(r"C:\__DATA__\Dropbox\01_PROJECTS\00_Personal\SteelFaceTools\dev\maya\docs\StickyLipsNode.md")
    viewer.show()
    sys.exit(app.exec())