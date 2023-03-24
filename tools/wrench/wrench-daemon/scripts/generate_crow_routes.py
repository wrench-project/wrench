import json, re, jsonref, sys



if __name__ == "__main__":

    if len(sys.argv) < 3:
        sys.stderr.write("Usage: " + sys.argv[0] + " <doc path> <include path>\n Generate crow routes header file from json\n")
        sys.exit(0)

    json_path = sys.argv[1]
    header_path = sys.argv[2]

    with open(json_path,"r") as f:
        data = jsonref.load(f)

    # initialize crows as a dict
    crows = {}

    '''
        transfrom paths into crow routes
        /simulation/{simid}/getTime -> /simulation/<string>/getTime

        To do this, check parameters/schema/type
    '''


    #Tempoary check: how many types are in wrench-openapi.json
    basket = set()
    for path in data["paths"].keys():

        method = list(data["paths"][path].keys())[0]

        crow_route = path

        routes = {}

        parameter_list = re.findall("\{(.*?)\}", path, re.I|re.M)

        operation = data["paths"][path][method]

        operationId = data["paths"][path][method]["operationId"]

        if 'parameters' in data["paths"][path][method].keys():
            # try to replace {} into exact type
            for parameter in data["paths"][path][method]["parameters"]:

                name = parameter['name']

                schema = parameter['schema']['type']

                crow_route = crow_route.replace('{'+name+'}', '<'+schema+'>', 1)


        if method in ('post', 'put'):
            if 'requestBody' in operation.keys():
                request_required = operation['requestBody']['required']
                schema = operation['requestBody']['content']['application/json']['schema']

                # create request body to represent the body params
                request_body = {}
                properties = schema['properties']
                for property in properties.keys():
                    request_body[property] = properties[property]['type']

                    if properties[property]['type'] == 'number' and 'format' in properties[property]:
                        request_body[property] = properties[property]['format']

                    basket.add(request_body[property])
                routes['request_required'] = request_required
                routes['request_body'] = request_body

                # Todo: add the parameter on the url to json body


        routes['method'] = method
        routes['parameter_list'] = parameter_list
        routes['operationId'] = operationId
        crows[crow_route] = routes

    '''
    Now to design route cpp code for each route

    '''
    apps = ""

    for crow in crows.keys():
        route = crows[crow]
        app = '\tCROW_ROUTE(app, "{0}").methods(crow::HTTPMethod::{1})\n'.format(crow, route['method'].upper())
        app += '\t\t'

        type_list = re.findall("\<(.*?)\>", crow, re.I|re.M)
        (route['parameter_list'], type_list)
        parameter_list = []

        parameter_list.append('const crow::request& req')
        for i in range(len(route['parameter_list'])):
            if type_list[i] == 'string':
                parameter_list.append('const std::string& '+route['parameter_list'][i])
            else:
                parameter_list.append('const '+type_list[i]+'& '+route['parameter_list'][i])
        parameter_str = ', '.join(parameter_list)
        app += '([this]({0}){{\n'.format(parameter_str)

        app += '\t\t\tjson req_json = json::parse(req.body);\n'
        for parameter_name in route['parameter_list']:
            app += '\t\t\treq_json[toStr({0})] = {0};\n'.format(parameter_name)

        app += '\t\t\tcrow::response res;\n'
        operationId = route['operationId']
        app += '\t\t\tthis->genericRequestHandler(req_json, res, "{0}");\n'.format(operationId)

        app += '\t\t\treturn res;\n'
        app += '\t\t});\n'

        apps += '\n'
        apps += app

    with open(header_path, 'w') as f:
        f.write(apps)







