
	CROW_ROUTE(app, "/simulation/startSimulation").methods(crow::HTTPMethod::Post)
		([this](const crow::request& req){
			json req_json = json::parse(req.body);
			crow::response res;
			this->genericRequestHandler(req_json, res, "startSimulation");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/getTime").methods(crow::HTTPMethod::Get)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "getTime");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/advanceTime").methods(crow::HTTPMethod::Put)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "advanceTime");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/createTask").methods(crow::HTTPMethod::Put)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "createTask");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/waitForNextSimulationEvent").methods(crow::HTTPMethod::Get)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "waitForNextSimulationEvent");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/simulationEvents").methods(crow::HTTPMethod::Get)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "simulationEvents");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/hostnames").methods(crow::HTTPMethod::Get)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "hostnames");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/inputFiles").methods(crow::HTTPMethod::Get)
		([this](const crow::request& req, const std::string& simid, const std::string& tid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			req_json[toStr(tid)] = tid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "inputFiles");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/addInputFile").methods(crow::HTTPMethod::Put)
		([this](const crow::request& req, const std::string& simid, const std::string& tid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			req_json[toStr(tid)] = tid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "addInputFile");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/addOutputFile").methods(crow::HTTPMethod::Put)
		([this](const crow::request& req, const std::string& simid, const std::string& tid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			req_json[toStr(tid)] = tid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "addOutputFile");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/inputFiles").methods(crow::HTTPMethod::Get)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "inputFiles");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/addFile").methods(crow::HTTPMethod::Put)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "addFile");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/files/<string>/size").methods(crow::HTTPMethod::Get)
		([this](const crow::request& req, const std::string& simid, const std::string& file_id){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			req_json[toStr(file_id)] = file_id;
			crow::response res;
			this->genericRequestHandler(req_json, res, "size");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/jobs/<string>/tasks").methods(crow::HTTPMethod::Get)
		([this](const crow::request& req, const std::string& simid, const std::string& job_name){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			req_json[toStr(job_name)] = job_name;
			crow::response res;
			this->genericRequestHandler(req_json, res, "tasks");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/taskGetFlops").methods(crow::HTTPMethod::Get)
		([this](const crow::request& req, const std::string& simid, const std::string& name){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			req_json[toStr(name)] = name;
			crow::response res;
			this->genericRequestHandler(req_json, res, "taskGetFlops");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/taskGetMinNumCores").methods(crow::HTTPMethod::Get)
		([this](const crow::request& req, const std::string& simid, const std::string& name){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			req_json[toStr(name)] = name;
			crow::response res;
			this->genericRequestHandler(req_json, res, "taskGetMinNumCores");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/taskGetMaxNumCores").methods(crow::HTTPMethod::Get)
		([this](const crow::request& req, const std::string& simid, const std::string& name){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			req_json[toStr(name)] = name;
			crow::response res;
			this->genericRequestHandler(req_json, res, "taskGetMaxNumCores");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/tasks/<string>/taskGetMemory").methods(crow::HTTPMethod::Get)
		([this](const crow::request& req, const std::string& simid, const std::string& name){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			req_json[toStr(name)] = name;
			crow::response res;
			this->genericRequestHandler(req_json, res, "taskGetMemory");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/createStandardJob").methods(crow::HTTPMethod::Put)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "createStandardJob");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/<string>/submit").methods(crow::HTTPMethod::Post)
		([this](const crow::request& req, const std::string& simid, const std::string& job_name){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			req_json[toStr(job_name)] = job_name;
			crow::response res;
			this->genericRequestHandler(req_json, res, "submit");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/addBareMetalComputeService").methods(crow::HTTPMethod::Post)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "addBareMetalComputeService");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/addCloudComputeService").methods(crow::HTTPMethod::Post)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "addCloudComputeService");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/addSimpleStorageService").methods(crow::HTTPMethod::Post)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "addSimpleStorageService");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/addFileRegistryService").methods(crow::HTTPMethod::Post)
		([this](const crow::request& req, const std::string& simid){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			crow::response res;
			this->genericRequestHandler(req_json, res, "addFileRegistryService");
			return res;
		});

	CROW_ROUTE(app, "/simulation/<string>/<string>/createFileCopy").methods(crow::HTTPMethod::Post)
		([this](const crow::request& req, const std::string& simid, const std::string& storage_service_name){
			json req_json = json::parse(req.body);
			req_json[toStr(simid)] = simid;
			req_json[toStr(storage_service_name)] = storage_service_name;
			crow::response res;
			this->genericRequestHandler(req_json, res, "createFileCopy");
			return res;
		});
