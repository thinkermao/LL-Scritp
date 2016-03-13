#include <iostream>
#include <stdexcept>
#include <memory>
#include <fstream>

#include <conio.h>
#include <windows.h>

#include "driver.h"
#include "Parser\lexer.h"
#include "Parser\Parser.h"
#include "Semantic\Analysis.h"
#include "Semantic\dumpAST.h"
#include "Semantic\Translator.h"
#include "Semantic\ASTContext.h"
#include "IR\QuadGenerator.h"
#include "IR\dumpQuad.h"
#include "IR\dumpCFG.h"
#include "IR\CodeGen.h"
#include "Runtime\opcode.h"
#include "Runtime\dumpOpcode.h"
#include "Runtime\VM.h"

int main(int argc, char* argv[])
{
    int length = 0;
    script::Byte *opcodes;
    {
        script::Driver &driver = script::Driver::instance();
        if (!driver.parseArguments(argc, argv))
            return 0;

        script::IRModule module;
        {
            script::ASTContext context;
            try {
                script::Lexer lexer;
                script::Parser parser(lexer, context);
                lexer.setProgram(std::string(driver.filename));
                parser.parse();

                script::Analysis analysis;
                analysis.analysis(context);
            }
            catch (std::runtime_error &e) {
                std::cout << e.what() << std::endl;
                return 0;
            }

            // Dump ast to file
            if (driver.dumpAST_)
            {
                std::string dumpFilename(driver.filename);
                dumpFilename += ".ast";
                std::fstream dumpASTFile(dumpFilename, std::ofstream::out);
                script::DumpAST dumpAST(dumpASTFile);
                dumpAST.dump(context);
            }

            // translate AST to IR (quad).
            script::Translator translator(module);
            translator.translate(context);
        }

        if (driver.dumpQuad_)
        {
            std::string dumpFilename(driver.filename);
            dumpFilename += ".quad";
            std::fstream dumpIRFile(dumpFilename, std::ofstream::out);
            script::DumpQuad dumpQuad(dumpIRFile);
            dumpQuad.dump(module);
        }

        if (driver.dumpCFG_)
        {
            std::string dumpFilename(driver.filename);
            dumpFilename += ".cfg";
            std::fstream dumpIRFile(dumpFilename, std::ofstream::out);
            script::DumpCFG dumpCFG(dumpIRFile);
            dumpCFG.dump(module);
        }

        script::OpcodeContext opcode;
        script::CodeGenerator codegen(opcode);
        codegen.gen(module);
        opcodes = opcode.getOpcodes(length);

        if (driver.dumpOpcode_)
        {
            std::string dumpFilename(driver.filename);
            dumpFilename += ".txt";
            std::fstream dumpOpcodeFile(dumpFilename, std::ofstream::out);
            script::DumpOpcode dumpByte(dumpOpcodeFile);
            dumpByte.dump(opcodes, length);
        }
    }

    try {
        script::VirtualMachine vm;
        auto beginTime = GetTickCount();
        vm.excute(opcodes, length);
        auto endTime = GetTickCount();
        std::cout << "�ܹ���ʱ: " << double(endTime - beginTime) << " ms" << std::endl;
    }
    catch (std::runtime_error &e) {
        std::cout << e.what() << std::endl;
        return 0;
    }

	return 0;
}