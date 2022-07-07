import requests
import json, sys, os.path, getpass, argparse


def create_argparser():
    parser = argparse.ArgumentParser()
    parser.add_argument("--KWProject", required=True)
    parser.add_argument("--KWView", required=True)
    parser.add_argument("--KWBuild", required=False)
    return parser

def getToken(serverHost, port, user) :
    ltoken = os.path.normpath(os.path.expanduser("~/.klocwork/ltoken"))
    ltokenFile = open(ltoken, 'r')
    for r in ltokenFile :
        rd = r.strip().split(';')
        if rd[0] == serverHost and rd[1] == str(port) and rd[2] == user :
            ltokenFile.close()
            return rd[3]
    ltokenFile.close()

def report(url, project, user, x, y, view, serverHost, port, build) :
    values = {"project": project, "user": user, "action": "report", "x": x, "y": y, "view": view}
    loginToken = getToken(serverHost, port, user)
    if loginToken is not None :
        values["ltoken"] = loginToken
    if build is not None :
        values["build"] = build
    response = requests.post(url, data=values)
    result = response.json()
    criticalCount = None
    errorCount = None
    warningCount = None
    reviewCount = None
    errorExists = False
    for index, value in enumerate(result["data"]) :
        name = result["rows"][index]["name"]
        print (name, "(s):", value[0])
        if "critical" in name.lower() :
            criticalCount = value[0]
        elif "error" in name.lower() :
            errorCount = value[0]

    if criticalCount is None and errorCount is None :
        if response.status_code != 200 :
            print ("Build is failed because there's no info about Criticals and Errors")
            sys.exit(-1)
    else :
        errorMsg = 'Build is failed because of '
        if criticalCount is not None and criticalCount > 0 :
            errorMsg += '{} Critical(s)'.format(criticalCount)
            errorExists = True
        if errorCount is not None and errorCount > 0 :
            if errorExists == True :
                errorMsg += ' and '
            errorMsg += '{} Error(s)'.format(errorCount)
            errorExists = True
        if errorExists == True :
            print (errorMsg)
            sys.exit(-1)
    print ("Build is complete!")

def main():
    arguments = create_argparser().parse_args()
    if (not arguments.KWBuild) or (arguments.KWBuild == "0") :
        build = None
    else:
        build = arguments.KWBuild 
    serverHost = "nnvklcwk002.inn.intel.com"
    host = "klocwork-inn6.devtools.intel.com"
    port = 8105
    user = getpass.getuser()
    project = arguments.KWProject
    view = arguments.KWView
    url = "https://%s:%d/review/api" % (host, port)
    x = "Severity"
    y = "Status"
    report(url, project, user, x, y, view, serverHost, port, build)

main()
