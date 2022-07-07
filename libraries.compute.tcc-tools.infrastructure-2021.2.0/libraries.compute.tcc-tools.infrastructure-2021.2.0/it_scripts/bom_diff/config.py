supported_package_types = {
    "LMT_Windows"    : ".*(Labeler|Labeling)_?Manager.*Windows.*\.zip",
    "LMT_Ubuntu16"   : ".*(Labeler|Labeling)_?Manager.*Ubuntu16.*\.zip",
    "LMT_Ubuntu14"   : ".*(Labeler|Labeling)_?Manager.*Ubuntu14.*\.zip",
    "LT_Windows"     : ".*LabelingTool.*Windows.*\.zip",
    "LT_Ubuntu16"    : ".*LabelingTool.*Ubuntu16.*\.zip",
    "LT_Ubuntu14"    : ".*LabelingTool.*Ubuntu14.*\.zip",
    "Plugin_sdk"     : ".*plugin_sdk..*\.zip",
    "Documentation"  : ".*documentation.*\.zip",
    "Getting_started": ".*getting_started.*\.zip",
    "Data_streams"   : ".*data_streams.*\.zip",
}

report_suffixes = [
    ("_Report_summary.html", "report_html_summary.txt", "Summary"),
    ("_Report.html"        , "report_html_diff.txt"   , "All Differences"),
    ("_Report_orphans.html", "report_html_orphans.txt", "Orphans"),
    ("_Report_changed.html", "report_html_changed.txt", "Changed only"),
    ("_Report_same.html"   , "report_html_same.txt"   , "Same"),
]

def createMainHtmlFiles(mainView, comparedPackages):
    create_main_menu_html(
        fileName="menu.html",
        mainView=mainView,
        comparedPackages=comparedPackages,
    )

    create_index_html(
        fileName="index.html",
        menuSrc="menu",
        indexSrc="index_{}".format(mainView),
        indexName="big_contents",
    )


def createHtmlForPackageType(packageType):
    create_menu_html(
        fileName="menu_{}.html".format(packageType),
        linkName="{}".format(packageType),
    )

    create_index_html(
        fileName="index_{}.html".format(packageType),
        menuSrc="menu_{}".format(packageType),
        indexSrc="{}_Report".format(packageType),
    )


def create_index_html(fileName, menuSrc, indexSrc, indexName="contents"):
    html = \
"""
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Frameset//EN"
   "http://www.w3.org/TR/html4/frameset.dtd">
<HTML>
    <HEAD>
        <TITLE>BOM Difference (Beyond Compare)</TITLE>
    </HEAD>
    <FRAMESET rows="30, 1*">
        <FRAME src="{MENU_SCR}.html" name="header">
        <FRAME src="{INDEX_SRC}.html" name="{INDEX_NAME}">
    </FRAMESET>
</HTML>
"""
    html = html.format(
        MENU_SCR=menuSrc,
        INDEX_SRC=indexSrc,
        INDEX_NAME=indexName,
    )
    with open(fileName, 'w') as fout:
        fout.write(html)


def create_menu_html(fileName, linkName):
    suffixesNames = [(suffix, friendlyName) for (suffix, config, friendlyName) in report_suffixes]
    links = ""
    for i in range(len(suffixesNames)):
        if i != 0:
            links += ' |\n'

        links += '    <a href="{LINK_NAME}{SUFFIX}" target="contents" >{NAME}</a>'.format(
            LINK_NAME=linkName,
            SUFFIX=suffixesNames[i][0],
            NAME=suffixesNames[i][1],
        )

    html =  \
"""
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Frameset//EN"
   "http://www.w3.org/TR/html4/frameset.dtd">
<HTML>
    <HEAD>
        <TITLE>navigation menu</TITLE>
    </HEAD>
{LINKS}
</HTML>
"""
    html = html.format(LINKS=links)
    with open(fileName, 'w') as fout:
        fout.write(html)


def create_main_menu_html(fileName, mainView, comparedPackages):
    linksNames = [mainView] + comparedPackages
    links = ""
    for i in range(len(linksNames)):
        if i != 0:
            links += ' |\n'
        links += '    <a href="index_{LINK}.html" target="big_contents" >{FRIENDLY_LINK}</a>'.format(
            LINK=linksNames[i],
            FRIENDLY_LINK=linksNames[i].replace("_", " "),
        )

    html = \
"""
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Frameset//EN"
   "http://www.w3.org/TR/html4/frameset.dtd">
<HTML>
<HEAD>
<TITLE>navigation menu</TITLE>
</HEAD>
{LINKS}
</HTML>
"""
    html = html.format(LINKS=links)
    with open(fileName, 'w') as fout:
        fout.write(html)
