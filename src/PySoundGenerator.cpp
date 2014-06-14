#include "PySoundGenerator.hpp"

/**
 * @brief PySoundGenerator::PySoundGenerator
 * @param progName
 * @param pyInstructions
 *
 * The constructor of the PySoundGeneratorclass.
 * Sets up the python interpreter, the instructions with
 * which it will be fed, the class variables and the break shortcut.
 */
PySoundGenerator::PySoundGenerator(char* progName, char* pyInstructions){
    if(pyInstructions == QString("")){
        emit doneSignal(PythonException("File is empty. Nothing to execute."));
        return;
    }
    qRegisterMetaType<PythonException>("PythonException");

    Py_SetProgramName(progName);
    Py_Initialize();
    triggered = false;
    ownExcept = PythonException();
    abortAction = new QAction(this);
    abortAction->setShortcut(QKeySequence("Ctrl-C"));
    connect(abortAction, SIGNAL(triggered()), this, SLOT(terminated()));
    PyObject* module = PyImport_AddModule("__main__");
    PyObject *sys = PyImport_ImportModule("sys");
    PyObject *path = PyObject_GetAttrString(sys, "path");
    PyList_Append(path, PyString_FromString("../../../"));
    PyList_Append(path, PyString_FromString("."));
    dict = PyModule_GetDict(module);
    execute(pyInstructions);
}

/**
 * @brief PySoundGenerator::~PySoundGenerator
 *
 * The destructor of the PySoundGenerator class.
 * Kills the python interpreter.
 */
PySoundGenerator::~PySoundGenerator(){
    delete abortAction;
    Py_Finalize();
}

/**
 * @brief PySoundGenerator::run
 *
 * The main loop. Calls the user code executing method
 * and emits a signal when it's done.
 */
void PySoundGenerator::run(){
    PyObject* check = execute("yield_wavefile(samples, None)");
    if(!check)
        exceptionOccurred();
    if(triggered)
        emit doneSignal(ownExcept);
}

/**
 * @brief PySoundGenerator::loop
 * @return PyObject* / NULL if there was an exception
 *         in the python interpreter.
 *
 * executes the python code in the interpreter.
 */
PyObject* PySoundGenerator::execute(QString instruct){
    return PyRun_String(instruct.toLocal8Bit().data(),
                        Py_file_input, dict, dict);
}

/**
 * @brief PySoundGenerator::updateCode
 * @param filename
 * @param instructions
 * @return true if the name of the program could be changed,
 *         false otherwise
 *
 * Updates the code of the currently running Python interpreter.
 */
bool PySoundGenerator::updateCode(QString filename, QString instructions){
    Py_SetProgramName(filename.toLocal8Bit().data());
    execute(instructions.toLocal8Bit().data());
    return true;
}

/**
 * @brief PySoundGenerator::exceptionOccurred
 * @return PythonException
 *
 * Fetches the Python Exception and translates it to a C++
 * exception.
 */
void PySoundGenerator::exceptionOccurred(){
    PyObject *errtype, *errvalue, *traceback;
    PyErr_Fetch(&errtype, &errvalue, &traceback);
    PyErr_NormalizeException(&errtype, &errvalue, &traceback);
    QString exceptionText = QString(PyString_AsString(PyObject_Str(errtype)));
    exceptionText.append(": ");
    exceptionText.append(PyString_AsString(PyObject_Str(errvalue)));
    triggered = true;
    ownExcept = PythonException(exceptionText);
}

/**
 * @brief PySoundGenerator::terminated
 *
 * SLOT that is called when the user interrupt(CTRL-C) SIGNAL
 * is emitted.
 */
void PySoundGenerator::terminated(){
    triggered = true;
    ownExcept = PythonException("User Terminated.");
}