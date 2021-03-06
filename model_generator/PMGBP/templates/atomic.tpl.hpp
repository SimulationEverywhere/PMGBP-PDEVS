/****************************** atomic model {{model_name}} *****************************************/

std::shared_ptr<cadmium::dynamic::modeling::model> {{model_name}} = 
	cadmium::dynamic::translate::make_dynamic_atomic_model<{{model_name}}_definition, {TIME}, {{ARGS}}>(
		"{{model_name}}",
		{{parameters}}
	);

/**************************************************************************************************/