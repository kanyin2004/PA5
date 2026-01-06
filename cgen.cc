#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <stack>

#include "cgen.h"
#include "cgen_gc.h"

extern void emit_string_constant(ostream& str, char* s);
extern int cgen_debug;

int labelnum = 0;
CgenClassTable* codegen_classtable = nullptr;

Symbol
    arg,
    arg2,
    Bool,
    concat,
    cool_abort,
    copy,
    Int,
    in_int,
    in_string,
    IO,
    length,
    Main,
    main_meth,
    No_class,
    No_type,
    Object,
    out_int,
    out_string,
    prim_slot,
    self,
    SELF_TYPE,
    Str,
    str_field,
    substr,
    type_name,
    val;

static void initialize_constants(void) {
    arg         = idtable.add_string("arg");
    arg2        = idtable.add_string("arg2");
    Bool        = idtable.add_string("Bool");
    concat      = idtable.add_string("concat");
    cool_abort  = idtable.add_string("abort");
    copy        = idtable.add_string("copy");
    Int         = idtable.add_string("Int");
    in_int      = idtable.add_string("in_int");
    in_string   = idtable.add_string("in_string");
    IO          = idtable.add_string("IO");
    length      = idtable.add_string("length");
    Main        = idtable.add_string("Main");
    main_meth   = idtable.add_string("main");
    No_class    = idtable.add_string("_no_class");
    No_type     = idtable.add_string("_no_type");
    Object      = idtable.add_string("Object");
    out_int     = idtable.add_string("out_int");
    out_string  = idtable.add_string("out_string");
    prim_slot   = idtable.add_string("_prim_slot");
    self        = idtable.add_string("self");
    SELF_TYPE   = idtable.add_string("SELF_TYPE");
    Str         = idtable.add_string("String");
    str_field   = idtable.add_string("_str_field");
    substr      = idtable.add_string("substr");
    type_name   = idtable.add_string("type_name");
    val         = idtable.add_string("_val");
}

static char* gc_init_names[] =
{ "_NoGC_Init", "_GenGC_Init", "_ScnGC_Init" };
static char* gc_collect_names[] =
{ "_NoGC_Collect", "_GenGC_Collect", "_ScnGC_Collect" };

BoolConst falsebool(FALSE);
BoolConst truebool(TRUE);

void program_class::cgen(ostream& os) {
    os << "# start of generated code\n";
    initialize_constants();
    codegen_classtable = new CgenClassTable(classes, os);
    codegen_classtable->Execute();
    os << "\n# end of generated code\n";
}

static void emit_load(const char* dest_reg, int offset, const char* source_reg, ostream& s) {
    s << LW << dest_reg << " " << offset* WORD_SIZE << "(" << source_reg << ")"
      << endl;
}

static void emit_store(const char* source_reg, int offset, const char* dest_reg, ostream& s) {
    s << SW << source_reg << " " << offset* WORD_SIZE << "(" << dest_reg << ")"
      << endl;
}

static void emit_load_imm(const char* dest_reg, int val, ostream& s) {
    s << LI << dest_reg << " " << val << endl;
}

static void emit_load_address(const char* dest_reg, const char* address, ostream& s) {
    s << LA << dest_reg << " " << address << endl;
}

static void emit_partial_load_address(const char* dest_reg, ostream& s) {
    s << LA << dest_reg << " ";
}

static void emit_load_bool(const char* dest, const BoolConst& b, ostream& s) {
    emit_partial_load_address(dest, s);
    b.code_ref(s);
    s << endl;
}

static void emit_load_string(const char* dest, StringEntry* str, ostream& s) {
    emit_partial_load_address(dest, s);
    str->code_ref(s);
    s << endl;
}

static void emit_load_int(const char* dest, IntEntry* i, ostream& s) {
    emit_partial_load_address(dest, s);
    i->code_ref(s);
    s << endl;
}

static void emit_move(const char* dest_reg, const char* source_reg, ostream& s) {
    s << MOVE << dest_reg << " " << source_reg << endl;
}

static void emit_neg(const char* dest, const char* src1, ostream& s) {
    s << NEG << dest << " " << src1 << endl;
}

static void emit_add(const char* dest, const char* src1, const char* src2, ostream& s) {
    s << ADD << dest << " " << src1 << " " << src2 << endl;
}

static void emit_addu(const char* dest, const char* src1, const char* src2, ostream& s) {
    s << ADDU << dest << " " << src1 << " " << src2 << endl;
}

static void emit_addiu(const char* dest, const char* src1, int imm, ostream& s) {
    s << ADDIU << dest << " " << src1 << " " << imm << endl;
}

static void emit_div(const char* dest, const char* src1, const char* src2, ostream& s) {
    s << DIV << dest << " " << src1 << " " << src2 << endl;
}

static void emit_mul(const char* dest, const char* src1, const char* src2, ostream& s) {
    s << MUL << dest << " " << src1 << " " << src2 << endl;
}

static void emit_sub(const char* dest, const char* src1, const char* src2, ostream& s) {
    s << SUB << dest << " " << src1 << " " << src2 << endl;
}

static void emit_sll(const char* dest, const char* src1, int num, ostream& s) {
    s << SLL << dest << " " << src1 << " " << num << endl;
}

static void emit_jalr(const char* dest, ostream& s) {
    s << JALR << "\t" << dest << endl;
}

static void emit_jal(const char* address, ostream& s) {
    s << JAL << address << endl;
}

static void emit_return(ostream& s) {
    s << RET << endl;
}

static void emit_gc_assign(ostream& s) {
    s << JAL << "_GenGC_Assign" << endl;
}

static void emit_disptable_ref(Symbol sym, ostream& s) {
    s << sym << DISPTAB_SUFFIX;
}

static void emit_init_ref(Symbol sym, ostream& s) {
    s << sym << CLASSINIT_SUFFIX;
}

static void emit_label_ref(int l, ostream& s) {
    s << "label" << l;
}

static void emit_protobj_ref(Symbol sym, ostream& s) {
    s << sym << PROTOBJ_SUFFIX;
}

static void emit_method_ref(Symbol classname, Symbol methodname, ostream& s) {
    s << classname << METHOD_SEP << methodname;
}

static void emit_label_def(int l, ostream& s) {
    emit_label_ref(l, s);
    s << ":" << endl;
}

static void emit_beqz(char* source, int label, ostream& s) {
    s << BEQZ << source << " ";
    emit_label_ref(label, s);
    s << endl;
}

static void emit_beq(const char* src1, const char* src2, int label, ostream& s) {
    s << BEQ << src1 << " " << src2 << " ";
    emit_label_ref(label, s);
    s << endl;
}

static void emit_bne(const char* src1, const char* src2, int label, ostream& s) {
    s << BNE << src1 << " " << src2 << " ";
    emit_label_ref(label, s);
    s << endl;
}

static void emit_bleq(const char* src1, const char* src2, int label, ostream& s) {
    s << BLEQ << src1 << " " << src2 << " ";
    emit_label_ref(label, s);
    s << endl;
}

static void emit_blt(const char* src1, const char* src2, int label, ostream& s) {
    s << BLT << src1 << " " << src2 << " ";
    emit_label_ref(label, s);
    s << endl;
}

static void emit_blti(const char* src1, int imm, int label, ostream& s) {
    s << BLT << src1 << " " << imm << " ";
    emit_label_ref(label, s);
    s << endl;
}

static void emit_bgti(const char* src1, int imm, int label, ostream& s) {
    s << BGT << src1 << " " << imm << " ";
    emit_label_ref(label, s);
    s << endl;
}

static void emit_branch(int l, ostream& s) {
    s << BRANCH;
    emit_label_ref(l, s);
    s << endl;
}

static void emit_push(char* reg, ostream& str) {
    emit_store(reg, 0, SP, str);
    emit_addiu(SP, SP, -4, str);
}

static void emit_fetch_int(const char* dest, char* source, ostream& s) {
    emit_load(dest, DEFAULT_OBJFIELDS, source, s);
}

static void emit_store_int(char* source, const char* dest, ostream& s) {
    emit_store(source, DEFAULT_OBJFIELDS, dest, s);
}

static void emit_test_collector(ostream& s) {
    emit_push(ACC, s);
    emit_move(ACC, SP, s);
    emit_move(A1, ZERO, s);
    s << JAL << gc_collect_names[cgen_Memmgr] << endl;
    emit_addiu(SP, SP, 4, s);
    emit_load(ACC, 0, SP, s);
}

static void emit_gc_check(char* source, ostream& s) {
    if (std::string(source) != std::string(A1)) {
        emit_move(A1, source, s);
    }
    s << JAL << "_gc_check" << endl;
}

void StringEntry::code_ref(ostream& s) {
    s << STRCONST_PREFIX << index;
}

void StringEntry::code_def(ostream& s, int stringclasstag) {
    IntEntryP lensym = inttable.add_int(len);
    s << WORD << "-1" << endl;
    code_ref(s);
    s  << LABEL
       << WORD << stringclasstag << endl
       << WORD << (DEFAULT_OBJFIELDS + STRING_SLOTS + (len + 4) / 4) << endl
       << WORD;
    s << Str << DISPTAB_SUFFIX;
    s << endl;
    s << WORD;
    lensym->code_ref(s);
    s << endl;
    emit_string_constant(s, str);
    s << ALIGN;
}

void StrTable::code_string_table(ostream& s, int stringclasstag) {
    for (List<StringEntry> *l = tbl; l; l = l->tl()) {
        l->hd()->code_def(s, stringclasstag);
    }
}

void IntEntry::code_ref(ostream& s) {
    s << INTCONST_PREFIX << index;
}

void IntEntry::code_def(ostream& s, int intclasstag) {
    s << WORD << "-1" << endl;
    code_ref(s);
    s << LABEL
      << WORD << intclasstag << endl
      << WORD << (DEFAULT_OBJFIELDS + INT_SLOTS) << endl
      << WORD;
    s << Int << DISPTAB_SUFFIX;
    s << endl;
    s << WORD << str << endl;
}

void IntTable::code_string_table(ostream& s, int intclasstag) {
    for (List<IntEntry> *l = tbl; l; l = l->tl()) {
        l->hd()->code_def(s, intclasstag);
    }
}

BoolConst::BoolConst(int i) : val(i) {
    assert(i == 0 || i == 1);
}

void BoolConst::code_ref(ostream& s) const {
    s << BOOLCONST_PREFIX << val;
}

void BoolConst::code_def(ostream& s, int boolclasstag) {
    s << WORD << "-1" << endl;
    code_ref(s);
    s << LABEL
      << WORD << boolclasstag << endl
      << WORD << (DEFAULT_OBJFIELDS + BOOL_SLOTS) << endl
      << WORD;
    s << Bool << DISPTAB_SUFFIX;
    s << endl;
    s << WORD << val << endl;
}

int Environment::AddObstacle() {
    EnterScope();
    return AddVar(No_class);
}

void CgenClassTable::code_global_data() {
    Symbol main    = idtable.lookup_string(MAINNAME);
    Symbol string  = idtable.lookup_string(STRINGNAME);
    Symbol integer = idtable.lookup_string(INTNAME);
    Symbol boolc   = idtable.lookup_string(BOOLNAME);

    str << "\t.data\n" << ALIGN;
    str << GLOBAL << CLASSNAMETAB << endl;
    str << GLOBAL;
    emit_protobj_ref(main, str);
    str << endl;
    str << GLOBAL;
    emit_protobj_ref(integer, str);
    str << endl;
    str << GLOBAL;
    emit_protobj_ref(string, str);
    str << endl;
    str << GLOBAL;
    falsebool.code_ref(str);
    str << endl;
    str << GLOBAL;
    truebool.code_ref(str);
    str << endl;
    str << GLOBAL << INTTAG << endl;
    str << GLOBAL << BOOLTAG << endl;
    str << GLOBAL << STRINGTAG << endl;

    str << INTTAG << LABEL
        << WORD << intclasstag << endl;
    str << BOOLTAG << LABEL
        << WORD << boolclasstag << endl;
    str << STRINGTAG << LABEL
        << WORD << stringclasstag << endl;
}

void CgenClassTable::code_global_text() {
    str << GLOBAL << HEAP_START << endl
        << HEAP_START << LABEL
        << WORD << 0 << endl
        << "\t.text" << endl
        << GLOBAL;
    emit_init_ref(idtable.add_string("Main"), str);
    str << endl << GLOBAL;
    emit_init_ref(idtable.add_string("Int"), str);
    str << endl << GLOBAL;
    emit_init_ref(idtable.add_string("String"), str);
    str << endl << GLOBAL;
    emit_init_ref(idtable.add_string("Bool"), str);
    str << endl << GLOBAL;
    emit_method_ref(idtable.add_string("Main"), idtable.add_string("main"), str);
    str << endl;
}

void CgenClassTable::code_bools(int boolclasstag) {
    falsebool.code_def(str, boolclasstag);
    truebool.code_def(str, boolclasstag);
}

void CgenClassTable::code_select_gc() {
    str << GLOBAL << "_MemMgr_INITIALIZER" << endl;
    str << "_MemMgr_INITIALIZER:" << endl;
    str << WORD << gc_init_names[cgen_Memmgr] << endl;
    str << GLOBAL << "_MemMgr_COLLECTOR" << endl;
    str << "_MemMgr_COLLECTOR:" << endl;
    str << WORD << gc_collect_names[cgen_Memmgr] << endl;
    str << GLOBAL << "_MemMgr_TEST" << endl;
    str << "_MemMgr_TEST:" << endl;
    str << WORD << (cgen_Memmgr_Test == GC_TEST) << endl;
}

void CgenClassTable::code_constants() {
    stringtable.add_string("");
    inttable.add_string("0");
    stringtable.code_string_table(str, stringclasstag);
    inttable.code_string_table(str, intclasstag);
    code_bools(boolclasstag);
}

void CgenClassTable::code_class_nameTab() {
    str << CLASSNAMETAB << LABEL;
    std::vector<CgenNode*> class_nodes = GetClassNodes();
    for (size_t i = 0; i < class_nodes.size(); ++i) {
        Symbol class_name = class_nodes[i]->name;
        StringEntry* str_entry = stringtable.lookup_string(class_name->get_string());
        str << WORD;
        str_entry->code_ref(str);
        str << endl;
    }
}

void CgenClassTable::code_class_objTab() {
    str << CLASSOBJTAB << LABEL;
    std::vector<CgenNode*> class_nodes = GetClassNodes();
    for (size_t i = 0; i < class_nodes.size(); ++i) {
        Symbol class_name = class_nodes[i]->name;
        StringEntry* str_entry = stringtable.lookup_string(class_name->get_string());
        str << WORD;
        emit_protobj_ref(str_entry, str);
        str << endl;
        str << WORD;
        emit_init_ref(str_entry, str);
        str << endl;
    }
}

void CgenClassTable::code_dispatchTabs() {
    std::vector<CgenNode*> class_nodes = GetClassNodes();
    for (size_t i = 0; i < class_nodes.size(); ++i) {
        CgenNode* current_node = class_nodes[i];
        emit_disptable_ref(current_node->name, str);
        str << LABEL;
        std::vector<method_class*> full_methods = current_node->GetFullMethods();
        std::map<Symbol, Symbol> dispatch_class_tab = current_node->GetDispatchClassTab();
        for (size_t j = 0; j < full_methods.size(); ++j) {
            Symbol method_name = full_methods[j]->name;
            Symbol class_name = dispatch_class_tab[method_name];
            str << WORD;
            emit_method_ref(class_name, method_name, str);
            str << endl;
        }
    }
}

std::vector<CgenNode*> CgenClassTable::GetClassNodes() {
    if (m_class_nodes.empty()) {
        std::vector<CgenNode*> temp_nodes;
        for (List<CgenNode> *l = nds; l; l = l->tl()) {
            temp_nodes.push_back(l->hd());
        }
        std::reverse(temp_nodes.begin(), temp_nodes.end());
        m_class_nodes = temp_nodes;
        for (size_t i = 0; i < m_class_nodes.size(); ++i) {
            m_class_nodes[i]->class_tag = i;
            m_class_tags[m_class_nodes[i]->get_name()] = i;
        }
    }
    return m_class_nodes;
}

std::map<Symbol, int> CgenClassTable::GetClassTags() {
    GetClassNodes();
    return m_class_tags;
}

std::vector<CgenNode*> CgenNode::GetInheritance() {
    if (inheritance.empty()) {
        std::vector<CgenNode*> temp_inheritance;
        CgenNode* current = this;
        while (current->name != No_class) {
            temp_inheritance.push_back(current);
            current = current->get_parentnd();
        }
        std::reverse(temp_inheritance.begin(), temp_inheritance.end());
        inheritance = temp_inheritance;
    }
    return inheritance;
}

std::vector<attr_class*> CgenNode::GetFullAttribs() {
    if (m_full_attribs.empty()) {
        std::vector<CgenNode*> inheritance_chain = GetInheritance();
        std::vector<attr_class*> temp_attribs;
        for (size_t i = 0; i < inheritance_chain.size(); ++i) {
            Features features = inheritance_chain[i]->features;
            for (int j = features->first(); features->more(j); j = features->next(j)) {
                Feature feature = features->nth(j);
                if (!feature->IsMethod()) {
                    temp_attribs.push_back((attr_class*)feature);
                }
            }
        }
        m_full_attribs = temp_attribs;
        for (size_t i = 0; i < m_full_attribs.size(); ++i) {
            m_attrib_idx_tab[m_full_attribs[i]->name] = i;
        }
    }
    return m_full_attribs;
}

std::vector<attr_class*> CgenNode::GetAttribs() {
    if (m_attribs.empty()) {
        std::vector<attr_class*> temp_attribs;
        for (int j = features->first(); features->more(j); j = features->next(j)) {
            Feature feature = features->nth(j);
            if (!feature->IsMethod()) {
                temp_attribs.push_back((attr_class*)feature);
            }
        }
        m_attribs = temp_attribs;
    }
    return m_attribs;
}

std::map<Symbol, int> CgenNode::GetAttribIdxTab() {
    GetFullAttribs();
    return m_attrib_idx_tab;
}

std::vector<method_class*> CgenNode::GetMethods() {
    if (m_methods.empty()) {
        std::vector<method_class*> temp_methods;
        for (int i = features->first(); features->more(i); i = features->next(i)) {
            Feature feature = features->nth(i);
            if (feature->IsMethod()) {
                temp_methods.push_back((method_class*)feature);
            }
        }
        m_methods = temp_methods;
    }
    return m_methods;
}

std::vector<method_class*> CgenNode::GetFullMethods() {
    if (m_full_methods.empty()) {
        std::vector<CgenNode*> inheritance_chain = GetInheritance();
        std::vector<method_class*> temp_methods;
        std::map<Symbol, int> temp_idx_tab;
        std::map<Symbol, Symbol> temp_class_tab;
        
        for (size_t i = 0; i < inheritance_chain.size(); ++i) {
            Symbol current_class_name = inheritance_chain[i]->name;
            std::vector<method_class*> current_methods = inheritance_chain[i]->GetMethods();
            for (size_t j = 0; j < current_methods.size(); ++j) {
                Symbol method_name = current_methods[j]->name;
                if (temp_idx_tab.find(method_name) == temp_idx_tab.end()) {
                    temp_methods.push_back(current_methods[j]);
                    temp_idx_tab[method_name] = temp_methods.size() - 1;
                    temp_class_tab[method_name] = current_class_name;
                } else {
                    int idx = temp_idx_tab[method_name];
                    temp_methods[idx] = current_methods[j];
                    temp_class_tab[method_name] = current_class_name;
                }
            }
        }
        m_full_methods = temp_methods;
        m_dispatch_idx_tab = temp_idx_tab;
        m_dispatch_class_tab = temp_class_tab;
    }
    return m_full_methods;
}

std::map<Symbol, Symbol> CgenNode::GetDispatchClassTab() {
    GetFullMethods();
    return m_dispatch_class_tab;
}

std::map<Symbol, int> CgenNode::GetDispatchIdxTab() {
    GetFullMethods();
    return m_dispatch_idx_tab;
}

void method_class::code(ostream& s, CgenNode* class_node) {
    emit_method_ref(class_node->name, name, s);
    s << LABEL;
    emit_addiu(SP, SP, -12, s);
    emit_store(FP, 3, SP, s);
    emit_store(SELF, 2, SP, s);
    emit_store(RA, 1, SP, s);
    emit_addiu(FP, SP, 4, s);
    emit_move(SELF, ACC, s);
    
    Environment env;
    env.m_class_node = class_node;
    for (int i = formals->first(); formals->more(i); i = formals->next(i)) {
        env.AddParam(formals->nth(i)->GetName());
    }
    expr->code(s, env);
    
    emit_load(FP, 3, SP, s);
    emit_load(SELF, 2, SP, s);
    emit_load(RA, 1, SP, s);
    emit_addiu(SP, SP, 12, s);
    emit_addiu(SP, SP, GetArgNum() * 4, s);
    emit_return(s);
}

void CgenNode::code_protObj(ostream& s) {
    std::vector<attr_class*> attribs = GetFullAttribs();
    s << WORD << "-1" << endl;
    s << get_name() << PROTOBJ_SUFFIX << LABEL;
    s << WORD << class_tag << endl;
    s << WORD << (DEFAULT_OBJFIELDS + attribs.size()) << endl;
    s << WORD << get_name() << DISPTAB_SUFFIX << endl;
    
    for (size_t i = 0; i < attribs.size(); ++i) {
        Symbol attr_name = attribs[i]->name;
        Symbol type = attribs[i]->type_decl;
        if (attr_name == val) {
            if (get_name() == Str) {
                s << WORD;
                inttable.lookup_string("0")->code_ref(s);
                s << endl;
            } else {
                s << WORD << "0" << endl;
            }
        } else if (attr_name == str_field) {
            s << WORD << "0" << endl;
        } else {
            if (type == Int) {
                s << WORD;
                inttable.lookup_string("0")->code_ref(s);
                s << endl;
            } else if (type == Bool) {
                s << WORD;
                falsebool.code_ref(s);
                s << endl;
            } else if (type == Str) {
                s << WORD;
                stringtable.lookup_string("")->code_ref(s);
                s << endl;
            } else {
                s << WORD << "0" << endl;
            }
        }
    }
}

void CgenNode::code_init(ostream& s) {
    s << get_name();
    s << CLASSINIT_SUFFIX;
    s << LABEL;
    emit_addiu(SP, SP, -12, s);
    emit_store(FP, 3, SP, s);
    emit_store(SELF, 2, SP, s);
    emit_store(RA, 1, SP, s);
    emit_addiu(FP, SP, 4, s);
    emit_move(SELF, ACC, s);
    
    Symbol parent_name = get_parentnd()->name;
    if (parent_name != No_class) {
        s << JAL;
        emit_init_ref(parent_name, s);
        s << endl;
    }

    std::vector<attr_class*> attribs = GetAttribs();
    std::map<Symbol, int> attrib_idx_tab = GetAttribIdxTab();
    for (size_t i = 0; i < attribs.size(); ++i) {
        int idx = attrib_idx_tab[attribs[i]->name];
        if (attribs[i]->init->IsEmpty()) {
            Symbol type = attribs[i]->type_decl;
            if (type == Str) {
                emit_load_string(ACC, stringtable.lookup_string(""), s);
                emit_store(ACC, 3 + idx, SELF, s);
            } else if (type == Int) {
                emit_load_int(ACC, inttable.lookup_string("0"), s);
                emit_store(ACC, 3 + idx, SELF, s);
            } else if (type == Bool) {
                emit_load_bool(ACC, BoolConst(0), s);
                emit_store(ACC, 3 + idx, SELF, s);
            }
        } else {
            Environment env;
            env.m_class_node = this;
            attribs[i]->init->code(s, env);
            emit_store(ACC, 3 + idx, SELF, s);
            if (cgen_Memmgr == 1) {
                emit_addiu(A1, SELF, 4 * (idx + 3), s);
                emit_jal("_GenGC_Assign", s);
            }
        }
    }

    emit_move(ACC, SELF, s);
    emit_load(FP, 3, SP, s);
    emit_load(SELF, 2, SP, s);
    emit_load(RA, 1, SP, s);
    emit_addiu(SP, SP, 12, s);
    emit_return(s);
}

void CgenNode::code_methods(ostream& s) {
    std::vector<method_class*> methods = GetMethods();
    for (size_t i = 0; i < methods.size(); ++i) {
        methods[i]->code(s, this);
    }
}

void CgenClassTable::code_protObjs() {
    std::vector<CgenNode*> class_nodes = GetClassNodes();
    for (size_t i = 0; i < class_nodes.size(); ++i) {
        class_nodes[i]->code_protObj(str);
    }
}

void CgenClassTable::code_class_inits() {
    std::vector<CgenNode*> class_nodes = GetClassNodes();
    for (size_t i = 0; i < class_nodes.size(); ++i) {
        class_nodes[i]->code_init(str);
    }
}

void CgenClassTable::code_class_methods() {
    std::vector<CgenNode*> class_nodes = GetClassNodes();
    for (size_t i = 0; i < class_nodes.size(); ++i) {
        if (!class_nodes[i]->basic()) {
            class_nodes[i]->code_methods(str);
        }
    }
}

CgenClassTable::CgenClassTable(Classes classes, ostream& s) : nds(NULL) , str(s) {
    enterscope();
    if (cgen_debug) {
        cout << "Building CgenClassTable" << endl;
    }
    install_basic_classes();
    install_classes(classes);
    build_inheritance_tree();
    std::map<Symbol, int> class_tags = GetClassTags();
    stringclasstag = class_tags[Str];
    intclasstag = class_tags[Int];
    boolclasstag = class_tags[Bool];
}

void CgenClassTable::install_basic_classes() {
    Symbol filename = stringtable.add_string("<basic class>");
    addid(No_class,
          new CgenNode(class_(No_class, No_class, nil_Features(), filename),
                       Basic, this));
    addid(SELF_TYPE,
          new CgenNode(class_(SELF_TYPE, No_class, nil_Features(), filename),
                       Basic, this));
    addid(prim_slot,
          new CgenNode(class_(prim_slot, No_class, nil_Features(), filename),
                       Basic, this));

    install_class(
        new CgenNode(
            class_(Object,
                   No_class,
                   append_Features(
                       append_Features(
                           single_Features(method(cool_abort, nil_Formals(), Object, no_expr())),
                           single_Features(method(type_name, nil_Formals(), Str, no_expr()))),
                       single_Features(method(copy, nil_Formals(), SELF_TYPE, no_expr()))),
                   filename),
            Basic, this));

    install_class(
        new CgenNode(
            class_(IO,
                   Object,
                   append_Features(
                       append_Features(
                           append_Features(
                               single_Features(method(out_string, single_Formals(formal(arg, Str)),
                                       SELF_TYPE, no_expr())),
                               single_Features(method(out_int, single_Formals(formal(arg, Int)),
                                       SELF_TYPE, no_expr()))),
                           single_Features(method(in_string, nil_Formals(), Str, no_expr()))),
                       single_Features(method(in_int, nil_Formals(), Int, no_expr()))),
                   filename),
            Basic, this));

    install_class(
        new CgenNode(
            class_(Int,
                   Object,
                   single_Features(attr(val, prim_slot, no_expr())),
                   filename),
            Basic, this));

    install_class(
        new CgenNode(
            class_(Bool, Object, single_Features(attr(val, prim_slot, no_expr())), filename),
            Basic, this));

    install_class(
        new CgenNode(
            class_(Str,
                   Object,
                   append_Features(
                       append_Features(
                           append_Features(
                               append_Features(
                                   single_Features(attr(val, Int, no_expr())),
                                   single_Features(attr(str_field, prim_slot, no_expr()))),
                               single_Features(method(length, nil_Formals(), Int, no_expr()))),
                           single_Features(method(concat,
                                           single_Formals(formal(arg, Str)),
                                           Str,
                                           no_expr()))),
                       single_Features(method(substr,
                                       append_Formals(single_Formals(formal(arg, Int)),
                                               single_Formals(formal(arg2, Int))),
                                       Str,
                                       no_expr()))),
                   filename),
            Basic, this));
}

void CgenClassTable::install_class(CgenNodeP nd) {
    Symbol name = nd->get_name();
    if (probe(name)) {
        return;
    }
    nds = new List<CgenNode>(nd, nds);
    addid(name, nd);
}

void CgenClassTable::install_classes(Classes cs) {
    for (int i = cs->first(); cs->more(i); i = cs->next(i)) {
        install_class(new CgenNode(cs->nth(i), NotBasic, this));
    }
}

void CgenClassTable::build_inheritance_tree() {
    for (List<CgenNode> *l = nds; l; l = l->tl()) {
        set_relations(l->hd());
    }
}

void CgenClassTable::set_relations(CgenNodeP nd) {
    CgenNode* parent_node = probe(nd->get_parent());
    nd->set_parentnd(parent_node);
    parent_node->add_child(nd);
}

void CgenNode::add_child(CgenNodeP n) {
    children = new List<CgenNode>(n, children);
}

void CgenNode::set_parentnd(CgenNodeP p) {
    assert(parentnd == NULL);
    assert(p != NULL);
    parentnd = p;
}

void CgenClassTable::code() {
    if (cgen_debug) {
        cout << "coding global data" << endl;
    }
    code_global_data();

    if (cgen_debug) {
        cout << "choosing gc" << endl;
    }
    code_select_gc();

    if (cgen_debug) {
        cout << "coding constants" << endl;
    }
    code_constants();

    if (cgen_debug) {
        cout << "coding name table" << endl;
    }
    code_class_nameTab();

    if (cgen_debug) {
        cout << "coding object table" << endl;
    }
    code_class_objTab();

    if (cgen_debug) {
        cout << "coding dispatch tables" << endl;
    }
    code_dispatchTabs();

    if (cgen_debug) {
        cout << "coding prototype objects" << endl;
    }
    code_protObjs();

    if (cgen_debug) {
        cout << "coding global text" << endl;
    }
    code_global_text();

    if (cgen_debug) {
        cout << "coding object initializers" << endl;
    }
    code_class_inits();

    if (cgen_debug) {
        cout << "coding class methods" << endl;
    }
    code_class_methods();
}

CgenNodeP CgenClassTable::root() {
    return probe(Object);
}

CgenNode::CgenNode(Class_ nd, Basicness bstatus, CgenClassTableP ct) :
    class__class((const class__class&) *nd),
    parentnd(NULL),
    children(NULL),
    basic_status(bstatus) {
    stringtable.add_string(name->get_string());
}

void assign_class::code(ostream& s, Environment env) {
    expr->code(s, env);
    int idx;
    bool is_found = false;

    if ((idx = env.LookUpVar(name)) != -1) {
        emit_store(ACC, idx + 1, SP, s);
        if (cgen_Memmgr == 1) {
            emit_addiu(A1, SP, 4 * (idx + 1), s);
            emit_jal("_GenGC_Assign", s);
        }
        is_found = true;
    }
    
    if (!is_found && (idx = env.LookUpParam(name)) != -1){
        emit_store(ACC, idx + 3, FP, s);
        if (cgen_Memmgr == 1) {
            emit_addiu(A1, FP, 4 * (idx + 3), s);
            emit_jal("_GenGC_Assign", s);
        }
        is_found = true;
    }
    
    if (!is_found && (idx = env.LookUpAttrib(name)) != -1) {
        emit_store(ACC, idx + 3, SELF, s);
        if (cgen_Memmgr == 1) {
            emit_addiu(A1, SELF, 4 * (idx + 3), s);
            emit_jal("_GenGC_Assign", s);
        }
        is_found = true;
    }
}

void static_dispatch_class::code(ostream& s, Environment env) {
    std::vector<Expression> actuals = GetActuals();
    Environment new_env = env;
    int actual_count = actuals.size();
    
    for (int i = 0; i < actual_count; ++i) {
        actuals[i]->code(s, env);
        emit_push(ACC, s);
        env.EnterScope();
        env.AddObstacle();
    }

    expr->code(s, env);
    emit_bne(ACC, ZERO, labelnum, s);
    s << LA << ACC << " str_const0" << endl;
    emit_load_imm(T1, 1, s);
    emit_jal("_dispatch_abort", s);
    emit_label_def(labelnum, s);
    ++labelnum;

    CgenNode* target_class = codegen_classtable->GetClassNode(type_name);
    std::string addr = type_name->get_string();
    addr += DISPTAB_SUFFIX;
    emit_load_address(T1, addr.c_str(), s);
    int method_idx = target_class->GetDispatchIdxTab()[name];
    emit_load(T1, method_idx, T1, s);
    emit_jalr(T1, s);
}

void dispatch_class::code(ostream& s, Environment env) {
    std::vector<Expression> actuals = GetActuals();
    int actual_count = actuals.size();
    
    for (int i = 0; i < actual_count; ++i) {
        actuals[i]->code(s, env);
        emit_push(ACC, s);
        env.AddObstacle();
    }

    expr->code(s, env);
    emit_bne(ACC, ZERO, labelnum, s);
    s << LA << ACC << " str_const0" << endl;
    emit_load_imm(T1, 1, s);
    emit_jal("_dispatch_abort", s);
    emit_label_def(labelnum, s);
    ++labelnum;

    Symbol target_class_name = env.m_class_node->name;
    if (expr->get_type() != SELF_TYPE) {
        target_class_name = expr->get_type();
    }

    CgenNode* target_class = codegen_classtable->GetClassNode(target_class_name);
    emit_load(T1, 2, ACC, s);
    int method_idx = target_class->GetDispatchIdxTab()[name];
    emit_load(T1, method_idx, T1, s);
    emit_jalr(T1, s);
}

void cond_class::code(ostream& s, Environment env) {
    pred->code(s, env);
    emit_fetch_int(T1, ACC, s);
    int label_false = labelnum++;
    int label_finish = labelnum++;
    emit_beq(T1, ZERO, label_false, s);
    then_exp->code(s, env);
    emit_branch(label_finish, s);
    emit_label_def(label_false, s);
    else_exp->code(s, env);
    emit_label_def(label_finish, s);
}

void loop_class::code(ostream& s, Environment env) {
    int label_start = labelnum++;
    int label_finish = labelnum++;
    emit_label_def(label_start, s);
    pred->code(s, env);
    emit_fetch_int(T1, ACC, s);
    emit_beq(T1, ZERO, label_finish, s);
    body->code(s, env);
    emit_branch(label_start, s);
    emit_label_def(label_finish, s);
    emit_move(ACC, ZERO, s);
}

void typcase_class::code(ostream& s, Environment env) {
    std::map<Symbol, int> class_tags = codegen_classtable->GetClassTags();
    std::vector<CgenNode*> class_nodes = codegen_classtable->GetClassNodes();
    
    expr->code(s, env);
    emit_bne(ACC, ZERO, labelnum, s);
    emit_load_address(ACC, "str_const0", s);
    emit_load_imm(T1, 1, s);
    emit_jal("_case_abort2", s);
    emit_label_def(labelnum, s);
    ++labelnum;

    emit_load(T1, 0, ACC, s);
    std::vector<branch_class*> branches = GetCases();
    int branch_count = branches.size();
    int label_base = labelnum;
    int label_finish = labelnum + branch_count;
    labelnum += branch_count + 1;

    auto get_child_tags = [&](std::vector<int> parent_tags) {
        std::vector<int> child_tags;
        for (size_t i = 0; i < parent_tags.size(); ++i) {
            CgenNode* parent_node = class_nodes[parent_tags[i]];
            std::vector<CgenNode*> children = parent_node->GetChildren();
            for (size_t j = 0; j < children.size(); ++j) {
                int child_tag = class_tags[children[j]->name];
                if (std::find(child_tags.begin(), child_tags.end(), child_tag) == child_tags.end()) {
                    child_tags.push_back(child_tag);
                }
            }
        }
        return child_tags;
    };

    auto has_unprocessed = [](std::vector<std::vector<int> > tag_sets) {
        for (size_t i = 0; i < tag_sets.size(); ++i) {
            if (!tag_sets[i].empty()) {
                return true;
            }
        }
        return false;
    };

    std::vector<std::vector<int> > branch_tags;
    for (int i = 0; i < branch_count; ++i) {
        Symbol type_decl = branches[i]->type_decl;
        int class_tag = class_tags[type_decl];
        std::vector<int> tag_set;
        tag_set.push_back(class_tag);
        branch_tags.push_back(tag_set);
    }

    while (has_unprocessed(branch_tags)) {
        for (int i = 0; i < branch_count; ++i) {
            std::vector<int> current_tags = branch_tags[i];
            for (size_t j = 0; j < current_tags.size(); ++j) {
                emit_load_imm(T2, current_tags[j], s);
                emit_beq(T1, T2, label_base + i, s);
            }
        }
        for (int i = 0; i < branch_count; ++i) {
            branch_tags[i] = get_child_tags(branch_tags[i]);
        }
    }

    emit_jal("_case_abort", s);
    emit_branch(label_finish, s);
    
    for (int i = 0; i < branch_count; ++i) {
        emit_label_def(label_base + i, s);
        env.EnterScope();
        env.AddVar(branches[i]->name);
        emit_push(ACC, s);
        branches[i]->expr->code(s, env);
        emit_addiu(SP, SP, 4, s);
        emit_branch(label_finish, s);
    }

    emit_label_def(label_finish, s);
}

void block_class::code(ostream& s, Environment env) {
    for (int i = body->first(); body->more(i); i = body->next(i)) {
        body->nth(i)->code(s, env);
    }
}

void let_class::code(ostream& s, Environment env) {
    init->code(s, env);
    if (init->IsEmpty()) {
        if (type_decl == Str) {
            emit_load_string(ACC, stringtable.lookup_string(""), s);
        } else if (type_decl == Int) {
            emit_load_int(ACC, inttable.lookup_string("0"), s);
        } else if (type_decl == Bool) {
            emit_load_bool(ACC, BoolConst(0), s);
        }
    }
    emit_push(ACC, s);
    env.EnterScope();
    env.AddVar(identifier);
    body->code(s, env);
    emit_addiu(SP, SP, 4, s);
}

void plus_class::code(ostream& s, Environment env) {
    e1->code(s, env);
    emit_push(ACC, s);
    env.AddObstacle();
    e2->code(s, env);
    emit_jal("Object.copy", s);
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);
    emit_move(T2, ACC, s);
    emit_load(T1, 3, T1, s);
    emit_load(T2, 3, T2, s);
    emit_add(T3, T1, T2, s);
    emit_store(T3, 3, ACC, s);
}

void sub_class::code(ostream& s, Environment env) {
    e1->code(s, env);
    emit_push(ACC, s);
    env.AddObstacle();
    e2->code(s, env);
    emit_jal("Object.copy", s);
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);
    emit_move(T2, ACC, s);
    emit_load(T1, 3, T1, s);
    emit_load(T2, 3, T2, s);
    emit_sub(T3, T1, T2, s);
    emit_store(T3, 3, ACC, s);
}

void mul_class::code(ostream& s, Environment env) {
    e1->code(s, env);
    emit_push(ACC, s);
    env.AddObstacle();
    e2->code(s, env);
    emit_jal("Object.copy", s);
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);
    emit_move(T2, ACC, s);
    emit_load(T1, 3, T1, s);
    emit_load(T2, 3, T2, s);
    emit_mul(T3, T1, T2, s);
    emit_store(T3, 3, ACC, s);
}

void divide_class::code(ostream& s, Environment env) {
    e1->code(s, env);
    emit_push(ACC, s);
    env.AddObstacle();
    e2->code(s, env);
    emit_jal("Object.copy", s);
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);
    emit_move(T2, ACC, s);
    emit_load(T1, 3, T1, s);
    emit_load(T2, 3, T2, s);
    emit_div(T3, T1, T2, s);
    emit_store(T3, 3, ACC, s);
}

void neg_class::code(ostream& s, Environment env) {
    e1->code(s, env);
    emit_jal("Object.copy", s);
    emit_load(T1, 3, ACC, s);
    emit_neg(T1, T1, s);
    emit_store(T1, 3, ACC, s);
}

void lt_class::code(ostream& s, Environment env) {
    e1->code(s, env);
    emit_push(ACC, s);
    env.AddObstacle();
    e2->code(s, env);
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);
    emit_move(T2, ACC, s);
    emit_load(T1, 3, T1, s);
    emit_load(T2, 3, T2, s);
    emit_load_bool(ACC, BoolConst(1), s);
    emit_blt(T1, T2, labelnum, s);
    emit_load_bool(ACC, BoolConst(0), s);
    emit_label_def(labelnum, s);
    ++labelnum;
}

void eq_class::code(ostream& s, Environment env) {
    e1->code(s, env);
    emit_push(ACC, s);
    env.AddObstacle();
    e2->code(s, env);
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);
    emit_move(T2, ACC, s);
    
    bool is_basic_type = (e1->type == Int || e1->type == Str || e1->type == Bool) &&
                         (e2->type == Int || e2->type == Str || e2->type == Bool);
    
    if (is_basic_type) {
        emit_load_bool(ACC, BoolConst(1), s);
        emit_load_bool(A1, BoolConst(0), s);
        emit_jal("equality_test", s);
    } else {
        emit_load_bool(ACC, BoolConst(1), s);
        emit_beq(T1, T2, labelnum, s);
        emit_load_bool(ACC, BoolConst(0), s);
        emit_label_def(labelnum, s);
        ++labelnum;
    }
}

void leq_class::code(ostream& s, Environment env) {
    e1->code(s, env);
    emit_push(ACC, s);
    env.AddObstacle();
    e2->code(s, env);
    emit_addiu(SP, SP, 4, s);
    emit_load(T1, 0, SP, s);
    emit_move(T2, ACC, s);
    emit_load(T1, 3, T1, s);
    emit_load(T2, 3, T2, s);
    emit_load_bool(ACC, BoolConst(1), s);
    emit_bleq(T1, T2, labelnum, s);
    emit_load_bool(ACC, BoolConst(0), s);
    emit_label_def(labelnum, s);
    ++labelnum;
}

void comp_class::code(ostream& s, Environment env) {
    e1->code(s, env);
    emit_load(T1, 3, ACC, s);
    emit_load_bool(ACC, BoolConst(1), s);
    emit_beq(T1, ZERO, labelnum, s);
    emit_load_bool(ACC, BoolConst(0), s);
    emit_label_def(labelnum, s);
    ++labelnum;
}

void int_const_class::code(ostream& s, Environment env) {
    emit_load_int(ACC, inttable.lookup_string(token->get_string()), s);
}

void string_const_class::code(ostream& s, Environment env) {
    emit_load_string(ACC, stringtable.lookup_string(token->get_string()), s);
}

void bool_const_class::code(ostream& s, Environment env) {
    emit_load_bool(ACC, BoolConst(val), s);
}

void new__class::code(ostream& s, Environment env) {
    if (type_name == SELF_TYPE) {
        emit_load_address(T1, "class_objTab", s);
        emit_load(T2, 0, SELF, s);
        emit_sll(T2, T2, 3, s);
        emit_addu(T1, T1, T2, s);
        emit_push(T1, s);
        emit_load(ACC, 0, T1, s);
        emit_jal("Object.copy", s);
        emit_load(T1, 1, SP, s);
        emit_addiu(SP, SP, 4, s);
        emit_load(T1, 1, T1, s);
        emit_jalr(T1, s);
        return;
    }

    std::string dest = type_name->get_string();
    dest += PROTOBJ_SUFFIX;
    emit_load_address(ACC, dest.c_str(), s);
    emit_jal("Object.copy", s);
    dest = type_name->get_string();
    dest += CLASSINIT_SUFFIX;
    emit_jal(dest.c_str(), s);
}

void isvoid_class::code(ostream& s, Environment env) {
    e1->code(s, env);
    emit_move(T1, ACC, s);
    emit_load_bool(ACC, BoolConst(1), s);
    emit_beq(T1, ZERO, labelnum, s);
    emit_load_bool(ACC, BoolConst(0), s);
    emit_label_def(labelnum, s);
    ++labelnum;
}

void no_expr_class::code(ostream& s, Environment env) {
    emit_move(ACC, ZERO, s);
}

void object_class::code(ostream& s, Environment env) {
    int idx;
    bool is_found = false;

    if ((idx = env.LookUpVar(name)) != -1) {
        emit_load(ACC, idx + 1, SP, s);
        if (cgen_Memmgr == 1) {
            emit_addiu(A1, SP, 4 * (idx + 1), s);
            emit_jal("_GenGC_Assign", s);
        }
        is_found = true;
    }
    
    if (!is_found && (idx = env.LookUpParam(name)) != -1) {
        emit_load(ACC, idx + 3, FP, s);
        if (cgen_Memmgr == 1) {
            emit_addiu(A1, FP, 4 * (idx + 3), s);
            emit_jal("_GenGC_Assign", s);
        }
        is_found = true;
    }
    
    if (!is_found && (idx = env.LookUpAttrib(name)) != -1) {
        emit_load(ACC, idx + 3, SELF, s);
        if (cgen_Memmgr == 1) {
            emit_addiu(A1, SELF, 4 * (idx + 3), s);
            emit_jal("_GenGC_Assign", s);
        }
        is_found = true;
    }
    
    if (!is_found && name == self) {
        emit_move(ACC, SELF, s);
    }
}