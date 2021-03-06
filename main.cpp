#include <bits/stdc++.h>

using namespace std;

enum operation_type {
    BINARY, UNARY
};
enum token {
    IMPL, OR, AND, NOT, BRACKET, VARIABLE, END
};

struct node {

    operation_type type;
    token op;
    string expression;

    shared_ptr<node> left, right;

    node() : left(nullptr), right(nullptr) { }
};


class parser {
    std::string expression;
    size_t pos;
    token cur_token;
    operation_type cur_operation_type;
    string cur_variable;

    void next_token() {
        while (pos < expression.length()) {
            switch (expression[pos]) {
                case ' ':
                    pos++;
                    continue;
                case '&' :
                    cur_operation_type = BINARY;
                    cur_token = AND;
                    break;
                case '|':
                    cur_operation_type = BINARY;
                    cur_token = OR;
                    break;
                case '!':
                    cur_operation_type = UNARY;
                    cur_token = NOT;
                    break;
                case '(':
                    cur_token = BRACKET;
                    break;
                case ')':
                    cur_token = BRACKET;
                    break;
                default:
                    break;
            }
            if (pos < expression.length() - 1) {
                if (expression[pos] == '-' && expression[pos + 1] == '>') {
                    cur_token = IMPL;
                    cur_operation_type = BINARY;
                    pos += 2;
                    return;
                }
            }
            string tmp;
            while ((pos < expression.length()) &&
                   (('A' <= expression[pos] && expression[pos] <= 'Z') ||
                    ('a' <= expression[pos] && expression[pos] <= 'z') ||
                    ('0' <= expression[pos] && expression[pos] <= '9'))) {
                cur_token = VARIABLE;
                tmp += expression[pos];
                pos++;
            }
            if (cur_token != VARIABLE) pos++;
            else cur_variable = tmp;
            return;
        }
        cur_token = END;
    }

    shared_ptr<node> expr() {
        next_token();
        shared_ptr<node> sub_root = disj();
        if (cur_token == IMPL) {
            shared_ptr<node> new_sub_root(new node());
            new_sub_root->left = sub_root;
            new_sub_root->right = expr();
            new_sub_root->op = IMPL;
            sub_root = new_sub_root;
        }
        while (cur_token == IMPL) {
            shared_ptr<node> right_children(new node());
            right_children->op = IMPL;
            right_children->left = sub_root->right;
            sub_root->right = right_children;
            right_children->right = expr();
        }
        return sub_root;
    }

    shared_ptr<node> disj() {
        shared_ptr<node> sub_root = conj();
        while (cur_token == OR) {
            next_token();
            shared_ptr<node> new_sub_root(new node());
            new_sub_root->op = OR;
            new_sub_root->left = sub_root;
            new_sub_root->right = conj();
            sub_root = new_sub_root;
        }
        return sub_root;
    }

    shared_ptr<node> conj() {
        shared_ptr<node> sub_root = negetion();
        while (cur_token == AND) {
            next_token();
            shared_ptr<node> new_sub_root(new node());
            new_sub_root->op = AND;
            new_sub_root->left = sub_root;
            new_sub_root->right = negetion();
            sub_root = new_sub_root;
        }
        return sub_root;
    }

    shared_ptr<node> negetion() {
        if (cur_token == VARIABLE) {
            shared_ptr<node> sub(new node());
            sub->op = VARIABLE;
            sub->expression = cur_variable;
            next_token();
            return sub;
        } else if (cur_token == NOT) {
            next_token();
            shared_ptr<node> sub(new node());
            sub->op = NOT;
            sub->left = negetion();
            return sub;
        } else if (cur_token == BRACKET) {
            shared_ptr<node> sub = expr();
            next_token();
            return sub;
        }
        return nullptr;
    }

    static string to_string(shared_ptr<node> u) {
        string ul(""), ur("");
        if (u->left != nullptr) {
            ul = to_string(u->left);
        }
        if (u->right != nullptr) {
            ur = to_string(u->right);
        }
        string sign;
        switch (u->op) {
            case IMPL:
                sign = "->";
                break;
            case OR:
                sign = "|";
                break;
            case AND:
                sign = "&";
                break;
            case NOT:
                sign = "!";
                break;
            default:
                break;
        }
        if (ur != "") {
            u->expression = "(" + ul + sign + ur + ")";
        } else if (ul != "") {
            u->expression = "(!" + ul + ")";
        }
        return u->expression;
    }

public:

    shared_ptr<node> parse(string &expression) {
        this->expression = expression;
        pos = 0;
        shared_ptr<node> root = expr();
        to_string(root);
        return root;
    }
};


vector<string> string_axioms = {"a->b->a",
                                "(a->b)->(a->b->c)->(a->c)",
                                "a->b->a&b",
                                "a&b->a",
                                "a&b->b",
                                "a->a|b",
                                "b->a|b",
                                "(a->c)->(b->c)->(a|b->c)",
                                "(a->b)->(a->!b)->!a",
                                "!!a->a"};

vector<string> assumptions;
string all_fun_is_for;

class checker {
    vector<shared_ptr<node>> axioms;

    bool check_nodes_structure(shared_ptr<node> v, shared_ptr<node> u) {
        if (v->left == nullptr && (v->right == nullptr)) {
            return u->left == nullptr && u->right == nullptr;
        }
        if (v->left == nullptr && v->right != nullptr) {
            return u->left == nullptr && u->right != nullptr;
        }
        if (v->left != nullptr && v->right == nullptr) {
            return u->left != nullptr && u->right == nullptr;
        }
        if (v->left != nullptr && v->right != nullptr) {
            return u->left != nullptr && u->right != nullptr;
        }
        return false;
    }

    bool check_mapped_expr(unordered_map<char, string> &m, char u, string &v) {
        if (m.count(u)) {
            return (m[u] == v);
        }
        m[u] = v;
        return true;
    }

    bool cur_axiom(shared_ptr<node> u, shared_ptr<node> root) {
        unordered_map<char, string> exprax_to_expr;
        queue<shared_ptr<node>> qax, q;
        qax.push(u);
        q.push(root);
        while (!qax.empty()) {
            shared_ptr<node> vax = qax.front();
            qax.pop();
            shared_ptr<node> v = q.front();
            q.pop();
            if (check_nodes_structure(vax, v)) {
                if (vax->op != v->op) return false;
                if (vax->op == VARIABLE) {
                    if (!check_mapped_expr(exprax_to_expr, vax->expression[0], v->expression)) return false;
                }
                if (vax->left != nullptr) {
                    qax.push(vax->left);
                    q.push(v->left);
                }
                if (vax->right != nullptr) {
                    qax.push(vax->right);
                    q.push(v->right);
                }
            } else {
                if (vax->left == nullptr && vax->right == nullptr) {
                    if (!check_mapped_expr(exprax_to_expr, vax->expression[0], v->expression)) return false;
                } else return false;
            }
        }
        return true;
    }

    unordered_map<string, int> all_we_have; //every expression TO number of line
    unordered_multimap<string, int> right_impl; //right node's expr in every tree with impl in root TO number of line
    unordered_map<int, string> left_impl; //number of line TO left node's expr of every tree with impl in root
    int line; //for counting lines
    string annotation_st;

public:

    checker() : line(0) {
        parser p;
        for (auto u: string_axioms) {
            axioms.push_back(p.parse(u));
        }
    }

    bool check_axioms(shared_ptr<node> root) {
        for (size_t i = 0; i < axioms.size(); i++) {
            if (cur_axiom(axioms[i], root)) {
                annotation_st = "Сх. акс. " + to_string(i + 1);
                return true;
            }
        }
        return false;
    }

    bool check_assumtions(shared_ptr<node> root) {
        string k = root->expression;
        for (size_t i = 0; i < assumptions.size(); i++) {
            if (assumptions[i] == k) {
                annotation_st = "Предп. " + to_string(i + 1);
                return true;
            }
        }
        return false;
    }


    bool check_MP(shared_ptr<node> root) {
        string k = root->expression;
        pair<std::unordered_multimap<string, int>::iterator, std::unordered_multimap<string, int>::iterator>
                beg_end = right_impl.equal_range(root->expression);
        for (auto it = beg_end.first; it != beg_end.second; it++) {
            int number_of_line = it->second;
            string left = left_impl.find(number_of_line)->second;
            auto it_number_second_line = all_we_have.find(left);
            if (it_number_second_line != all_we_have.end()) {
                annotation_st = "M.P. " + to_string(it_number_second_line->second) + " " + to_string(number_of_line);
                return true;
            }
        }
        return false;
    }

    bool check(shared_ptr<node> root) {
        line++;
        if (check_axioms(root) || check_assumtions(root) || check_MP(root)) {
            all_we_have.insert(make_pair(root->expression, line));
            if (root->op == IMPL) {
                left_impl.insert(make_pair(line, root->left->expression));
                right_impl.insert(make_pair(root->right->expression, line));
            }
            return true;
        }
        annotation_st = "Не доказано";
        return false;
    }

    string get_annotation() {
        return annotation_st;
    }

    int get_line_number() {
        return line;
    }

};

void assumptions_go(parser &p, string &s) {
    string tmp("");
    s += ' ';
    for (size_t i = 0; i < s.length() - 1; i++) {
        if (s[i] == ' ') {
            continue;
        } else if (s[i] != ',' && !(s[i] == '|' && s[i + 1] == '-')) {
            tmp += s[i];
        } else {
            if (tmp != "") {
                assumptions.push_back(p.parse(tmp)->expression);
                tmp = "";
            }
            if ((s[i] == '|' && s[i + 1] == '-')) i++;
        }
    }
    if (tmp != "") {
        all_fun_is_for = p.parse(tmp)->expression;
    }
}

int main() {
    freopen("good6.in", "r", stdin);
    freopen("out.txt", "w", stdout);
    setlocale(LC_ALL, "Russian");
    time(0);
    clock_t t_beg = clock();
    string s;
    getline(cin, s);
    parser p;
    assumptions_go(p, s);

    checker ch;

    cout << s << endl;
    while (getline(cin, s)) {
        ch.check(p.parse(s));
        printf("(%d) %s (%s)\n", ch.get_line_number(), s.data(), ch.get_annotation().data());
    }

    clock_t t_end = clock();
    //cout << t_end - t_beg;


    return 0;
}