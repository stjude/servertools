//
// ServerTools
// Copyright (C) 2012 David Coss, PhD
//
// You should have received a copy of the GNU General Public License
// in the file COPYING.  If not, see <http://www.gnu.org/licenses/>.

//
// This source code provides utility functions for embedding Python in
// BOINC validation and assimilation code. Here, the BoincResult class
// is defined. This is used to pass some of the data in the  RESULT 
// struct from the BOINC API to the embedded Python code.
//
// Usage: 
// Replace "init_filename" with the path of the initialization Python
// code. This code should create a dict to map Application IDs (appid),
// as strings, to the name of Python functions to call.
//
// List of dict's needed are: validators, cleaners and assimilators
// Note: It is acceptable for these dict's to be empty. If the Python
// function does not find an entry for the appid being used, it will skip
// that function.
//

#include <Python.h>
#include <sstream>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pyboinc.h"

#include "boinc/validate_util.h"
#include "boinc/boinc_db_types.h"

#include <vector>
#include <string>
#include <structmember.h>
#include <stdarg.h>


// Debugging and error logging
// If DEBUG is defined, debugging is done.
// Else, only error logging is done.
#ifdef DEBUG
#define WHERESTR  "[%s:%d] "
#define WHEREARG  __FILE__, __LINE__
#define DEBUGPRINT ERRORPRINT
#else
#define WHERESTR  "%s "
#define WHEREARG  "ERROR:"
#define DEBUGPRINT(_fmt,...) 
#endif
#define DEBUGPRINT2(...)       fprintf(stderr, __VA_ARGS__)
#define ERRORPRINT(_fmt, ...) DEBUGPRINT2(WHERESTR _fmt, WHEREARG, __VA_ARGS__)
static void BoincResult_dealloc(BoincResult *self)
{
  if(self == NULL)
    return;

  printf("Dealloc'ed \n");
  printf("Name(%s) ref count: %d\n",PyBytes_AsString(self->name),(int)self->name->ob_refcnt);
  Py_XDECREF(self->name);
  printf("Output file list ref count: %d\n",(int)self->output_files->ob_refcnt);
  Py_XDECREF(self->output_files);
  //self->ob_type->tp_free((PyObject*)self);
  Py_TYPE(self)->tp_free((PyObject*)self);
  
  
}

static PyMemberDef pyboinc_RESULT_members[] = {
  {"name", T_OBJECT_EX, offsetof(BoincResult,name)},
  {"output_files", T_OBJECT_EX, offsetof(BoincResult,output_files)},
  {"id", T_INT, offsetof(BoincResult,id)},
  {"appid", T_INT, offsetof(BoincResult,appid)},
  {"exit_status", T_INT, offsetof(BoincResult,exit_status)},
  {"validate_state", T_INT, offsetof(BoincResult,validate_state)},
  {"cpu_time", T_DOUBLE, offsetof(BoincResult,cpu_time)},
  {NULL}
};

static PyTypeObject pyboinc_RESULT = {
  PyObject_HEAD_INIT(NULL)
  0,
  "BoincResult",
  sizeof(BoincResult),
  0,                         /*tp_itemsize*/
  (destructor)BoincResult_dealloc,                         /*tp_dealloc*/
  0,                         /*tp_print*/
  0,                         /*tp_getattr*/
  0,                         /*tp_setattr*/
  0,                         /*tp_compare*/
  0,                         /*tp_repr*/
  0,                         /*tp_as_number*/
  0,                         /*tp_as_sequence*/
  0,                         /*tp_as_mapping*/
  0,                         /*tp_hash */
  0,                         /*tp_call*/
  0,                         /*tp_str*/
  0,                         /*tp_getattro*/
  0,                         /*tp_setattro*/
  0,                         /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT,        /*tp_flags*/
  "Boinc Result Object",           /* tp_doc */
};

static PyObject* BoincResult_new(PyTypeObject *type, PyObject *args, PyObject *kwrds)
{
  BoincResult *obj;
  obj = (BoincResult*)type->tp_alloc(type,0);
  if(obj == NULL)
    return (PyObject*)obj;

  obj->name = PyBytes_FromString("UNINITIALIZED");
  if(obj->name == NULL)
    {
      printf("Could not initialize the string: name\n");
      Py_DECREF(obj);
      return NULL;
    }

  obj->output_files = PyList_New(0);
  if(obj->output_files == NULL)
    {
      printf("Could not initialize the list: output_files\n");
      Py_DECREF(obj);
      return NULL;
    }

  obj->appid = 0;
  obj->id = 0;
  obj->exit_status = 0;
  obj->cpu_time = 0.0;
  obj->validate_state = 0;

  printf("When created, result name (%s) ref count: %d\n",PyBytes_AsString(obj->name),(int)obj->name->ob_refcnt);

  
  return (PyObject *)obj;
  
}


// Returns a new reference
static int BoincResult_init(BoincResult *self, PyObject *args, PyObject *kwds)
{
  
  PyObject *name=NULL,  *tmp = NULL;
  int id, appid, exit_status,validate_state;
  double cpu_time;
  static char *kwlist[] = {"name","appid","exit_status","validate_state","id","cpu_time",NULL};


  if(!PyArg_ParseTupleAndKeywords(args,kwds,"|Oiiid",kwlist,&name,&appid,&exit_status,&id,&cpu_time))
    return -1;

  if(name)
    {
      tmp = self->name;
      Py_INCREF(name);
      self->name = name;
      Py_XDECREF(tmp);
    }

  self->appid = appid;
  self->id = appid;
  self->exit_status = exit_status;
  self->cpu_time = cpu_time;
  self->validate_state = validate_state;

  return 0;
  
}

// Returns a new reference
PyObject* RESULT2BoincResult(const RESULT& result)
{
  PyObject *boincresult, *tmp, *new_name;
  BoincResult *the_struct = NULL;

  boincresult = BoincResult_new(&pyboinc_RESULT,NULL,NULL);
  if(boincresult == NULL)
    return NULL;

  the_struct = (BoincResult*)boincresult;

    // Set name
  printf("Initial result name (%s) ref count: %d\n",PyBytes_AsString(the_struct->name),the_struct->name->ob_refcnt);
  tmp = the_struct->name;
  new_name = PyBytes_FromString(result.name);
  printf("When new name is created (%s) ref count: %d\n",PyBytes_AsString(new_name),new_name->ob_refcnt);

  the_struct->name = new_name;
  printf("New result name (%s) ref count: %d\n",PyBytes_AsString(the_struct->name),the_struct->name->ob_refcnt);
  Py_INCREF(the_struct->name);
  Py_XDECREF(tmp);

  // app id
  the_struct->appid = result.appid;
  the_struct->exit_status = result.exit_status;
  the_struct->cpu_time = result.cpu_time;
  the_struct->validate_state = result.validate_state;
  the_struct->id = result.id;

  return boincresult;
  
}

// Returns a borrowed reference
PyObject* import_result(PyObject *module, const char *variable_name, const std::vector<std::string> *paths, const RESULT& result)
{
  PyObject *retval = NULL;
  BoincResult *the_struct = NULL;

  if(module == NULL || variable_name == NULL)
    {
      fprintf(stderr,"NULL pointer for module or variable name\n");
      Py_INCREF(Py_None);
      return Py_None;
    }

  retval = RESULT2BoincResult(result);
  if(retval == NULL)
    {
      fprintf(stderr,"Error occurred importing result.");
      if(PyErr_Occurred())
	PyErr_Print();
      Py_INCREF(Py_None);
      return Py_None;
    }
  the_struct = (BoincResult*)retval;

  
  if(paths != NULL)
    {
      std::string logical_name,physical_name;
      RESULT non_const_res = result;
      PyObject *args;
      for(std::vector<std::string>::const_iterator path = paths->begin();path != paths->end();path++)
	{
	  physical_name = *path;
	  if(get_logical_name(non_const_res,physical_name,logical_name) != 0)
	    printf("WARNING -- Could not get logical name for %s\n",physical_name.c_str());
	  printf("GOT PATHS: %s, %s\n",path->c_str(),logical_name.c_str());
	  args = Py_BuildValue("(ss)",path->c_str(),logical_name.c_str());
	  PyList_Append(the_struct->output_files,args);
	  Py_XDECREF(args);
	}
    }

  PyModule_AddObject(module,variable_name,retval);

  return (PyObject*)retval;
}

int init_boinc_result(PyObject *module)
{
  // NULL Check
  if(module == NULL)
    return 0;
  
  // Check to see if the class is already in the module
  PyObject *item;
  PyObject *mod_dict = PyModule_GetDict(module);// borrowed reference
  item = PyDict_GetItemString(mod_dict,pyboinc_RESULT.tp_name);// borrowed reference
  if(item != NULL)
    return 0;// already loaded
  
  pyboinc_RESULT.tp_new = BoincResult_new;
  pyboinc_RESULT.tp_init = (initproc)BoincResult_init;// __init__
  pyboinc_RESULT.tp_members = pyboinc_RESULT_members;

  if(PyType_Ready(&pyboinc_RESULT) < 0)
    {
      fprintf(stderr,"Could not ready pyboinc_RESULT\n");
      return -1;
    }

  if(module == NULL)
    return 0;

  Py_INCREF(&pyboinc_RESULT);
  PyModule_AddObject(module, pyboinc_RESULT.tp_name, (PyObject *)&pyboinc_RESULT);

  return 0;

}

PyObject *py_user_code_on_results(int num_results, const RESULT *r1, void* _data1, RESULT const *r2, void* _data2, const char *function_dict_name)
{
  int retval = 0;
  PyObject *main_module = NULL, *main_dict = NULL, *validator_funct = NULL, *funct_name = NULL, *funct_dict = NULL, *valid_value = NULL, *appid_obj = NULL;
  PyObject *pyresult1, *pyresult2;
  FILE *init_file = NULL;
  const char init_filename[] = "/boinc/projects/stjudeathome/boincdag_init.py";
  
  if(function_dict_name == NULL)
    {
      fprintf(stderr,"Missing function name\n"); 
      Py_INCREF(Py_None);
      return Py_None;
    }
  if(num_results < 1 || num_results > 2)
    {
      fprintf(stderr,"Invalid number of results for py_user_code_on_results.\n");
      Py_INCREF(Py_None);
      return Py_None;
    }
  if(r1 == NULL || _data1 == NULL)
    {
      fprintf(stderr,"Missing result or data value.\n");
      Py_INCREF(Py_None);
      return Py_None;
    }
  if(num_results == 2 && (r2 == NULL || _data2 == NULL))
    {
      fprintf(stderr,"Missing second result or data value.\n");
      Py_INCREF(Py_None);
      return Py_None;
    }

  init_file = fopen(init_filename,"r");
  if(init_file == NULL)
    {
      char cwd[FILENAME_MAX];
      strcpy(cwd,"UNKNOWN");
      if(getcwd(cwd,FILENAME_MAX))
	{
	  fprintf(stderr,"Could not get current working directory. Reason: %s\n",strerror(errno));
	}
      fprintf(stderr,"Could not open %s for reading.\nCWD: %s\n",init_filename,cwd);
      fprintf(stderr,"Reason: %s\n",strerror(errno));
      Py_INCREF(Py_None);
      return Py_None;
    }

  main_module = PyImport_AddModule("__main__");
  if(main_module == NULL)
    {
      fprintf(stderr,"Could not add module __main__\n");
      if(errno)
	fprintf(stderr,"Reason: %s\n",strerror(errno));
      Py_INCREF(Py_None);
      return Py_None;
    }

  init_boinc_result(main_module);
    
  main_dict = PyModule_GetDict(main_module);
  if(main_dict == NULL)
    {
      fprintf(stderr,"Could not get globals.\n");
      if(errno)
	fprintf(stderr,"Reason: %s\n",strerror(errno));
      Py_INCREF(Py_None);
      return Py_None;
    }

  pyresult1 = import_result(main_module,"result1",(const std::vector<std::string>*)_data1,*r1);
  if(num_results == 2)
    pyresult2 = import_result(main_module,"result2",(const std::vector<std::string>*)_data2,*r2);
    
  retval = PyRun_SimpleFile(init_file,init_filename);
  fclose(init_file);init_file = NULL;
  if(retval)
    {
      fprintf(stderr,"Error running %s.\n",init_filename);
      Py_INCREF(Py_None);
      return Py_None;
    }
  
  funct_dict = PyObject_GetAttrString(main_module,function_dict_name);// dictionary that maps app ids to function names
  if(funct_dict == NULL)
    {
      ERRORPRINT("Missing %s dict.\n",function_dict_name);
      Py_INCREF(Py_None);
      return Py_None;
    }
  appid_obj = PyUnicode_FromFormat("%d",r1->appid);
  funct_name = PyDict_GetItem(funct_dict, appid_obj);// function name for this appid
  Py_DECREF(funct_dict);
  Py_XDECREF(appid_obj);
  if(funct_name == NULL)
    {
      fprintf(stderr,"Missing %s for %d.\n",function_dict_name,r1->appid);
      Py_INCREF(Py_None);
      return Py_None;
    }
  validator_funct = PyObject_GetAttr(main_module, funct_name);
  Py_XDECREF(funct_name);
  if(validator_funct == NULL)
    {
      fprintf(stderr,"Missing %s function '%s' for %d.\n",function_dict_name,PyBytes_AsString(funct_name),r1->appid);
      Py_INCREF(Py_None);
      return Py_None;
    }
  if(!PyCallable_Check(validator_funct))
    {
      fprintf(stderr,"Object is not callable.\n");
      Py_XDECREF(validator_funct);
      Py_INCREF(Py_None);
      return Py_None;
    }

  if(num_results == 2)
    {
      PyObject *args = Py_BuildValue("(OO)",pyresult1, pyresult2);
      valid_value = PyObject_CallObject(validator_funct,args);
      Py_XDECREF(args);
    }
  else
    {
      PyObject *args = Py_BuildValue("(O)",pyresult1);
      valid_value = PyObject_CallObject(validator_funct,args);
      Py_XDECREF(args);
    }

  if(PyErr_Occurred())
    {
      Py_XDECREF(validator_funct);
      Py_XDECREF(valid_value);
      PyErr_Print();
      Py_INCREF(Py_None);
      return Py_None;
    }

  if(valid_value == NULL)
    {
      PyObject *name_obj = PyObject_GetAttrString(validator_funct,"__name__");
      if(name_obj == NULL)
	name_obj = PyBytes_FromString("NULL");
      fprintf(stderr,"Error running %s (%s).\n",function_dict_name,PyBytes_AsString(name_obj));
      Py_XDECREF(name_obj);
      Py_XDECREF(validator_funct);
      Py_XDECREF(valid_value);
      Py_INCREF(Py_None);
      return Py_None;
    }
  Py_XDECREF(validator_funct);

  return valid_value;// returning new reference
}

PyObject *py_user_code_on_workunit(std::vector<RESULT>& results, RESULT *canonical_result, const char *function_dict_name)
{
  int retval = 0, appid;
  PyObject *main_module = NULL, *main_dict = NULL, *funct_to_run = NULL, *funct_name = NULL, *funct_dict = NULL,*valid_value = NULL, *boinctools_mod = NULL, *args = NULL, *appid_obj = NULL;
  PyObject *boinctools_name = NULL;
  PyObject *boinc_results = NULL;// List of boinc results corresponding to results argument
  PyObject *pycanonical = NULL;// BoincResult for canonical result.
  std::vector<RESULT>::const_iterator res_it;
  FILE *init_file = NULL;
  const char init_filename[] = "/boinc/projects/stjudeathome/boincdag_init.py";
  
  if(function_dict_name == NULL)
    {
      Py_INCREF(Py_None);
      return Py_None;
    }

  if(canonical_result != NULL)
    appid = canonical_result->appid;
  else if(results.size() > 0)
    appid = results[0].appid;
  else
    appid = -1;
  
  init_file = fopen(init_filename,"r");
  if(init_file == NULL)
    {
      char cwd[FILENAME_MAX];
      strcpy(cwd,"UNKNOWN");
      if(getcwd(cwd,FILENAME_MAX))
	{
	  fprintf(stderr,"Could not get current working directory. Reason: %s\n",strerror(errno));
	}
      fprintf(stderr,"Could not open %s for reading.\nCWD: %s\n",init_filename,cwd);
      fprintf(stderr,"Reason: %s\n",strerror(errno));
      Py_INCREF(Py_None);
      return Py_None;
    }

  main_module = PyImport_AddModule("__main__");
  if(main_module == NULL)
    {
      fprintf(stderr,"Could not add module __main__\n");
      if(errno)
	fprintf(stderr,"Reason: %s\n",strerror(errno));
     
      Py_INCREF(Py_None);
      return Py_None;
    }

  init_boinc_result(main_module);
    
  main_dict = PyModule_GetDict(main_module);
  if(main_dict == NULL)
    {
      fprintf(stderr,"Could not get globals.\n");
      if(errno)
	fprintf(stderr,"Reason: %s\n",strerror(errno));
      Py_INCREF(Py_None);
      return Py_None;
    }

  boinctools_name = PyBytes_FromString("boinctools");
  boinctools_mod = PyImport_Import(boinctools_name);
  Py_XDECREF(boinctools_name);boinctools_name = NULL;
  if(boinctools_mod == NULL)
    {
      fprintf(stderr,"ERROR - Could not load boinctools python module.\n");
      exit(1);
    }

  // Create result list
  boinc_results = PyList_New(0);
  if(boinc_results == NULL)
    {
      fprintf(stderr,"Could not create result list in assimilate_handler.\n");
      if(PyErr_Occurred())
	PyErr_Print();
      Py_DECREF(boinctools_mod);
      Py_INCREF(Py_None);
      return Py_None;
    }
  for(res_it = results.begin();res_it != results.end();res_it++)
    {
      PyObject *pyresult = RESULT2BoincResult(*res_it);
      if(pyresult == NULL)
	{
	  fprintf(stderr,"Error occurred importing result.");
	  if(PyErr_Occurred())
	    PyErr_Print();

	  Py_DECREF(boinc_results);
	  Py_DECREF(boinctools_mod);
	  Py_INCREF(Py_None);
	  return Py_None;
	}
      PyList_Append(boinc_results,pyresult);
    }

  if(canonical_result == NULL)
    {
      Py_INCREF(Py_None);
      pycanonical = Py_None;
    }
  else
    {
      // CHECK THIS FOR Py_INC/DECREF
      pycanonical = RESULT2BoincResult(*canonical_result);
    }
    
  retval = PyRun_SimpleFile(init_file,init_filename);
  fclose(init_file);init_file = NULL;
  if(retval)
    {
      fprintf(stderr,"Error running %s.\n",init_filename);
      Py_DECREF(boinc_results);
      Py_DECREF(boinctools_mod);
      Py_INCREF(Py_None);
      return Py_None;
    }
  
  funct_dict = PyObject_GetAttrString(main_module,function_dict_name);// new reference. dict that maps app ids to function names
  if(funct_dict == NULL)
    {
      ERRORPRINT("Missing %s dict.\n",function_dict_name);
      Py_DECREF(boinc_results);
      Py_DECREF(boinctools_mod);
      Py_INCREF(Py_None);
      return Py_None;
    }
  appid_obj = PyUnicode_FromFormat("%d",appid);
  funct_name = PyDict_GetItem(funct_dict, appid_obj);// borrowed reference
  Py_DECREF(funct_dict);
  Py_XDECREF(appid_obj);
  if(funct_name == NULL)
    {
      DEBUGPRINT("Missing %s for %d.\n",function_dict_name,appid);
      Py_DECREF(boinc_results);
      Py_DECREF(boinctools_mod);
      Py_INCREF(Py_None);
      return Py_None;
    }
  funct_to_run = PyObject_GetAttr(main_module, funct_name);// new reference
  if(funct_to_run == NULL)
    {
      ERRORPRINT("Missing %s function '%s' for %d.\n",function_dict_name,PyBytes_AsString(funct_name),appid);
      Py_DECREF(funct_name);
      Py_DECREF(boinctools_mod);
      Py_DECREF(boinc_results);
      Py_INCREF(Py_None);
      return Py_None;
    }
  Py_DECREF(funct_name);

  if(!PyCallable_Check(funct_to_run))
    {
      fprintf(stderr,"Object is not callable.\n");
      Py_DECREF(funct_to_run);
      Py_DECREF(boinc_results);
      Py_DECREF(boinctools_mod);
      Py_INCREF(Py_None);
      return Py_None;
    }

  args = Py_BuildValue("(OO)",boinc_results,pycanonical);
  valid_value = PyObject_CallObject(funct_to_run,args);
  Py_XDECREF(args);
  if(PyErr_Occurred())
    {
      Py_XDECREF(valid_value);
      PyErr_Print();
      Py_DECREF(funct_to_run);
      Py_DECREF(boinc_results);
      Py_INCREF(Py_None);
      return Py_None;
    }

  if(valid_value == NULL)
    {
      PyObject *name_obj = PyObject_GetAttrString(funct_to_run,"__name__");
      if(name_obj == NULL)
	name_obj = PyBytes_FromString("NULL");
      fprintf(stderr,"Error running %s (%s).\n",function_dict_name,PyBytes_AsString(name_obj));
      Py_XDECREF(name_obj);
      Py_DECREF(funct_to_run);
      Py_DECREF(boinctools_mod);
      Py_DECREF(boinc_results);
      Py_INCREF(Py_None);
      return Py_None;
    }

  Py_DECREF(funct_to_run);
  Py_DECREF(boinctools_mod);
  Py_DECREF(boinc_results);
  Py_DECREF(pycanonical);

  return valid_value;
}


PyObject* py_boinctools_on_result(const RESULT& r, const char *function_name)
{
  PyObject *mod = NULL, *mod_name = NULL;
  PyObject *result = NULL;
  const char *module_name = "boinctools";

  if(function_name == NULL)
    {
      Py_INCREF(Py_None);
      return Py_None;
    }

  mod_name = PyBytes_FromString(module_name);
  mod = PyImport_Import(mod_name);
  Py_XDECREF(mod_name);mod_name = NULL;
  if(mod != NULL)
    {
      PyObject *dict, *funct;
      dict = PyModule_GetDict(mod);
      funct = PyDict_GetItemString(dict,function_name);
      if(funct != NULL)
	{
	  if(PyCallable_Check(funct))
	    {
	      PyObject *exception = NULL;
	      result = PyObject_CallFunction(funct,"(s)",r.name);
	      if((exception = PyErr_Occurred()) != NULL)
		{
		  const char *exc_name = PyBytes_AsString(PyObject_GetAttrString(exception,"__name__"));
		  if(exc_name == NULL || strcmp(exc_name,"NoSuchProcess") != 0)
		    {
		      Py_XDECREF(result);
		      Py_XDECREF(mod);
		      Py_INCREF(Py_None);
		      return Py_None;
		    }
		}
	    }
	}
      Py_XDECREF(mod);
    }

  return result;
}


PyObject* py_boinctools_on_workunit(const std::vector<RESULT>& results, const RESULT *canonical_result, const char *function_name)
{
  PyObject *mod = NULL, *module_name_obj = NULL;
  PyObject *boinc_results = NULL, *pycanonical = NULL;
  PyObject *result = NULL;
  const char *module_name = "boinctools";
  std::vector<RESULT>::const_iterator res_it;

  if(function_name == NULL)
    {
      Py_INCREF(Py_None);
      return Py_None;
    }

  // Create result list
  boinc_results = PyList_New(0);
  if(boinc_results == NULL)
    {
      fprintf(stderr,"Could not create result list in assimilate_handler.\n");
      if(PyErr_Occurred())
	PyErr_Print();
      return Py_None;
    }
  for(res_it = results.begin();res_it != results.end();res_it++)
    {
      PyObject *pyresult = RESULT2BoincResult(*res_it);
      if(pyresult == NULL)
	{
	  fprintf(stderr,"Error occurred importing result.");
	  if(PyErr_Occurred())
	    PyErr_Print();
	  Py_DECREF(boinc_results);
	  Py_INCREF(Py_None);
	  return Py_None;
	}
      PyList_Append(boinc_results,pyresult);
    }

  if(canonical_result == NULL)
    {
      pycanonical = Py_None;
      Py_INCREF(pycanonical);
    }
  else
    {
      pycanonical = RESULT2BoincResult(*canonical_result);
    }

  module_name_obj = PyBytes_FromString(module_name);
  mod = PyImport_Import(module_name_obj);
  Py_DECREF(module_name_obj);
  if(mod != NULL)
    {
      PyObject *dict, *funct;
      dict = PyModule_GetDict(mod);
      funct = PyDict_GetItemString(dict,function_name);
      if(funct != NULL)
	{
	  if(PyCallable_Check(funct))
	    {
	      PyObject *exception = NULL;
	      result = PyObject_CallFunction(funct,"(OO)",boinc_results,pycanonical);
	      if((exception = PyErr_Occurred()) != NULL)
		{
		  const char *exc_name = PyBytes_AsString(PyObject_GetAttrString(exception,"__name__"));
		  if(exc_name == NULL || strcmp(exc_name,"NoSuchProcess") != 0)
		    {
		      Py_XDECREF(pycanonical);
		      Py_DECREF(boinc_results);
		      Py_XDECREF(mod);
		      Py_XDECREF(result);
		      Py_INCREF(Py_None);
		      return Py_None;
		    }
		}
	    }
	}
    }
  Py_XDECREF(mod);
  Py_XDECREF(pycanonical);
  Py_DECREF(boinc_results);

  return result;
}


void initialize_python()
{
  if(Py_IsInitialized())
    return;
  
  Py_Initialize();
}

void finalize_python()
{
  if(!Py_IsInitialized())
    return;
  
  Py_Finalize();
}

std::string result_init_string(const RESULT& res)
{
  std::ostringstream buffer;
  buffer << " boinctools.BoincResult(\"" << res.name << "\", " << res.id << ", " << res.appid << ", " << res.exit_status << ", " << res.validate_state << ", " << res.cpu_time << ") ";
  return buffer.str();
}

void load_paths(const std::string& variable_name, const RESULT& res, void *data)
{
  std::vector<std::string> *paths = (std::vector<std::string>*)data;

  if(paths != NULL)
    {
      std::string logical_name,physical_name;
      std::ostringstream buffer;
      RESULT non_const_res = res;
      for(std::vector<std::string>::const_iterator path = paths->begin();path != paths->end();path++)
	{
	  physical_name = *path;
	  if(get_logical_name(non_const_res,physical_name,logical_name) != 0)
	    printf("WARNING -- Could not get logical name for %s\n",physical_name.c_str());
	  buffer.clear();buffer.str("");
	  buffer << variable_name << ".output_files.append((\"" << path->c_str() << "\", \"" << logical_name.c_str() << "\"))";
	  if(PyRun_SimpleString(buffer.str().c_str()))
	    {
	      fprintf(stderr,"Could not create result object.\n");
	      fprintf(stderr,"Python Command: %s",buffer.str().c_str());
	      if(PyErr_Occurred())
		PyErr_Print();
	      finalize_python();
	      exit(1);	      
	    }
	}
    }
}



