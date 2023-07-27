#ifndef ANIMTOOL_CLI_H
#define ANIMTOOL_CLI_H

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstring>
#include <memory.h>
#include <stdarg.h>

#define MAX_N_FLAGS 100
#define MAX_N_CMDS 100
#define MAX_N_CMD_EXAMPLES 50
#define MAX_N_FLAG_VALUES 10
#define MAX_N_FLAG_ALIASES 5
#define MAX_N_ARGS 100


namespace cli {
    enum FlagType {
        FLAG_INT,
        FLAG_BOOL,
        FLAG_STR,
        FLAG_FLOAT
    };


    struct StrBuilder {
        char buf[10240];
        char* cur;

        StrBuilder(): cur(buf) {

        }

        void AddTextVargs(const char* format, va_list ap) {
            int n = vsnprintf(cur, buf + sizeof(buf) - cur, format, ap);
            if (n >= 0) {
                cur += n;
            }
        }


        void AddText(const char* format, ...) {
            va_list ap;
            va_start(ap, format);
            AddTextVargs(format, ap);
            va_end(ap);
        }

        void AddLine(const char* format, ...) {
            va_list ap;
            va_start(ap, format);
            AddTextVargs(format, ap);
            va_end(ap);
            AddText("\n");
        }
    };

    static inline void printerr(const char* format, ...) {
        StrBuilder sb;
        va_list ap;
        va_start(ap, format);
        sb.AddTextVargs(format, ap);
        va_end(ap);

        sb.AddLine("");

        fprintf(stderr, "ERROR: %s", sb.buf);
    }

    static inline const char* GetFlagTypeName(FlagType type) {
        switch (type) {
            case FLAG_BOOL: return "BOOL";
            case FLAG_INT: return "INT";
            case FLAG_STR: return "STR";
            case FLAG_FLOAT: return "FLOAT";
            default: return nullptr;
        }
    }

    static inline int StrToBool(const char* value) {
        if (value == nullptr) {
            return 0;
        }

        if (!strcasecmp(value, "false") || !strcmp(value, "0")) {
            return 0;
        }

        return 1;
    }

    struct FlagValue {
        union  {
            int int_value;
            const char* str_value;
            float float_value;
            int bool_value;
        };
    };

    static inline int StrToFlagValue(const char* str, FlagType type, FlagValue* value) {
        switch (type) {
            case FLAG_BOOL: value->bool_value = StrToBool(str); break;
            case FLAG_INT: value->int_value = atoi(str); break;
            case FLAG_FLOAT: value->float_value = atof(str); break;
            case FLAG_STR: value->str_value = str; break;
            default: return -1;
        }

        return 0;
    }

    static inline int FlagValueToStr(char* buf, int buf_len, FlagType type, const FlagValue* value) {
        switch (type) {
            case FLAG_INT: snprintf(buf, buf_len, "%d", value->int_value); break;
            case FLAG_FLOAT: snprintf(buf, buf_len, "%.1f", value->float_value); break;
            case FLAG_STR: 
                if (value->str_value) 
                    snprintf(buf, buf_len, "`%s`", value->str_value); 
                else
                    snprintf(buf, buf_len, "``"); 
                break;
            default: return -1;
        }

        return 0;
    }

    struct FlagResult {
        const char* name;
        FlagType type;
        int n_values;
        FlagValue values[MAX_N_FLAG_VALUES];

        void AddValue(FlagValue value) {
            if (n_values >= MAX_N_FLAG_VALUES) {
                return;
            }

            memcpy(&values[n_values], &value, sizeof(value));
            ++n_values;
        }
    };


    struct Flag {
        const char* name;
        char short_aliases[MAX_N_FLAG_ALIASES];
        const char* desc;
        FlagType type;
        int required;
        int multiple;
        FlagValue default_value;

        int ValueExpected() const {
            return type != FLAG_BOOL;
        }

        int Matches(const char* arg) const {
            if (strlen(arg) > 2 && arg[0] == '-' && arg[1] == '-') {
                return !strcmp(name, arg+2);
            } else if (arg[0] == '-') {
                for (int i=0; i<MAX_N_FLAG_ALIASES; ++i) {
                    if (short_aliases[i] == *(arg + 1))
                        return 1;
                }
                return 0;
            } else {
                return 0;
            }
        }
    };

    struct CmdResult {
        int n_flags;
        FlagResult flags[MAX_N_FLAGS];

        int n_args;
        const char* args[MAX_N_ARGS];

        int GetResult(const char* name, FlagResult* result) const {
            for (int i=0; i<n_flags; ++i) {
                auto& flag = flags[i];
                if (!strcmp(flag.name, name)) {
                    if (result)
                        memcpy(result, &flag, sizeof(FlagResult));

                    return 0;
                }
            }
            return -1;
        }


        int GetResultWithValue(const char* name, FlagType type, FlagResult* out_result) const {
            FlagResult result{};
            if (GetResult(name, &result)) {
                if (out_result)
                    printerr("AF: flag not found --%s", name);
                return -1;
            }

            if (result.type != type) {
                printerr("AF: flag type not matched --%s", name);
                return -1;
            }

            if (result.n_values == 0) {
                if (out_result)
                    printerr("AF: flag no value --%s", name);
                return -1;
            }

            if (out_result)
                memcpy(out_result, &result, sizeof(result));

            return 0;
        }

        int GetInt(const char* name) const {
            FlagResult result{};
            if (GetResultWithValue(name, FLAG_INT, &result)) {
                return 0;
            }

            return result.values[0].int_value;
        }

        int GetFloat(const char* name) const {
            FlagResult result{};
            if (GetResultWithValue(name, FLAG_FLOAT, &result)) {
                return 0;
            }

            return result.values[0].float_value;
        }

        const char* GetStr(const char* name) const {
            FlagResult result{};
            if (GetResultWithValue(name, FLAG_STR, &result)) {
                return nullptr;
            }

            return result.values[0].str_value;
        }


        int GetStrList(const char* name, const char** buf, int* n_str) const {
            FlagResult result{};
            if (GetResultWithValue(name, FLAG_STR, &result)) {
                return -1;
            }

            for (int i=0; i<result.n_values; ++i) {
                buf[i] = result.values[i].str_value;
            }

            *n_str = result.n_values;

            return 0;
        }

        int GetBool(const char* name) const {
            FlagResult result{};
            if (GetResultWithValue(name, FLAG_BOOL, &result)) {
                return 0;
            }

            return result.values[0].bool_value;
        }

        int AddFlagValue(const Flag* flag, FlagValue flag_value) {
            for (int i=0; i<n_flags; ++i) {
                if (!strcmp(flags[i].name, flag->name)) {
                    if (!flag->multiple) {
                        printerr("Multiple value %s", flag->name);
                        return -1;
                    }

                    flags[i].AddValue(flag_value);
                    return 0;
                }
            }

            if (n_flags >= MAX_N_FLAGS) {
                printerr("MAX_N_FLAGS reached %s", flag->name);
                return -1;
            }

            FlagResult result {
                .name = flag->name,
                .type = flag->type
            };

            result.AddValue(flag_value);

            memcpy(&flags[n_flags], &result, sizeof(FlagResult));
            ++n_flags;

            return 0;
        }

        int AddFlagRawValue(const Flag* flag, const char* raw_value) {
            FlagValue fv{};
            if (StrToFlagValue(raw_value, flag->type, &fv)) {
                printerr("StrToFlagValue --%s", flag->name);
                return -1;
            }

            if (AddFlagValue(flag, fv)) {
                printerr("AddFlagValue --%s", flag->name);
                return -1;
            }
            return 0;
        }

        int AddArg(const char* arg) {
            if (n_args >= MAX_N_ARGS) {
                printerr("MAX_N_ARGS reached");
                return -1;
            }

            args[n_args] = arg;
            ++n_args;
            return 0;
        }

        const char* GetFirstArg() const {
            if (n_args == 0) return nullptr;

            return args[0];
        }
    };

    enum ActionError {
        ACTION_OK = 0,
        ACTION_WRONG_ARGS = -1,
        ACTION_FAILED = -2,
    };


    struct Cmd {
        const char* name;
        const char* desc;
        const char* usage;
        const char* examples[MAX_N_CMD_EXAMPLES];
        int n_args;
        const char* args_desc;

        void* context;
        ActionError (*action)(void* context, const CmdResult* cmd, cli::StrBuilder& error);

        int n_flags;
        Flag flags[MAX_N_FLAGS];

        void GetHelpStr(StrBuilder& sb, const char* app) const {
            sb.AddLine("NAME:");
            sb.AddLine("    %s - %s", name, desc);
            sb.AddLine("");
            sb.AddLine("USAGE:");
            if (usage) {
                sb.AddLine("    %s %s %s", app, name, usage);
            } else {
                if (n_args == 0) {
                    sb.AddLine("    %s %s [command options]", app, name);
                } else if (n_args == 1) {
                    sb.AddLine("    %s %s [command options] argument", app, name);
                } else {
                    sb.AddLine("    %s %s [command options] arguments...", app, name);
                }
            }
            if (examples[0]) {
                sb.AddLine("");
                sb.AddLine("EXAMPLES:");
                for (int i=0; i<MAX_N_CMD_EXAMPLES; ++i) {
                    auto& example = examples[i];
                    if (example) {
                        sb.AddLine("    %s", example);
                    }
                }
            }
            sb.AddLine("");
            sb.AddLine("OPTIONS:");
            for (int i=0; i<n_flags; ++i) {
                auto& flag = flags[i];
                sb.AddText("    --%s", flag.name);
                for (int j=0; j<MAX_N_FLAG_ALIASES; ++j) {
                    auto& alias = flag.short_aliases[j];
                    if (alias) {
                        sb.AddText(", -%s", alias);
                    }
                }

                sb.AddText(" %s", GetFlagTypeName(flag.type));
                if (!flag.required) {
                    if (flag.ValueExpected()) {
                        char buf[256] = {};
                        if (!FlagValueToStr(buf, sizeof(buf), flag.type, &flag.default_value)) {
                            sb.AddText(", optional, default to %s", buf);
                        }
                    }
                }

                if (flag.multiple) {
                    sb.AddText(", multiple");
                }

                sb.AddText("\n");
                sb.AddText("        %s", flag.desc);
                sb.AddText("\n");
            }

            if (n_args > 0) {
                sb.AddLine("");
                sb.AddLine("ARGUMENTS:");
                sb.AddLine("    %d arguments required.", n_args);
                if (args_desc)
                    sb.AddLine("    %s", args_desc);

            }
        }

        void PrintHelp(const char* app) const {
            StrBuilder sb;
            GetHelpStr(sb, app);
            fprintf(stderr, "%s\n", sb.buf);
        }

        void AddFlag(Flag flag) {
            if (n_flags >= MAX_N_FLAGS)
                return;

            flags[n_flags++] = flag;
        }

        const Flag* FindFlag(const char* arg) const {
            if (strlen(arg) <= 1) return nullptr;
            if (arg[0] != '-') return nullptr;

            for (int i=0; i<n_flags; ++i) {
                auto& flag = flags[i];
                if (flag.Matches(arg)) {
                    return &flag;
                }
            }

            return nullptr;
        }

        int Parse(int argc, char *argv[], CmdResult* result) {
            const Flag* unfinished_flag = nullptr;
            for (int i=0; i< argc; ++i) {
                auto& arg = argv[i];

                auto flag = FindFlag(arg);
                if (flag) { // flag
                    if (unfinished_flag) {
                        printerr("Missing option value for --%s", unfinished_flag->name);
                        return -1;
                    }

                    if (flag->ValueExpected()) {
                        unfinished_flag = flag;
                    } else {
                        if (result->AddFlagRawValue(flag, "true")) {
                            printerr("Failed AddFlagRawValue --%s", flag->name);
                            return -1;
                        }
                    }
                } else { // value
                    if (unfinished_flag) {
                        if (result->AddFlagRawValue(unfinished_flag, arg)) {
                            printerr("Failed AddFlagRawValue --%s %s", unfinished_flag->name, arg);
                            return -1;
                        }
                        unfinished_flag = nullptr;
                    } else {
                        result->AddArg(arg);
                    }
                }
            }

            if (unfinished_flag) {
                printerr("Missing option value --%s", unfinished_flag->name);
                return -1;
            }
            
            for (int i=0; i<n_flags; ++i) {
                auto& flag = flags[i];
                if (result->GetResultWithValue(flag.name, flag.type, nullptr)) {
                    if (flag.required) {
                        printerr("Missing option --%s", flag.name);
                        return -1;
                    }
                    if (result->AddFlagValue(&flag, flag.default_value)) {
                        printerr("AddFlagValue --%s", flag.name);
                        return -1;
                    }
                }
            }

            if (result->n_args < n_args) {
                printerr("%d arguments required but only %d provided.", n_args, result->n_args);
                return -1;
            }

            return 0;
        }
    };


    struct App {
        const char* name;
        const char* desc;
        int n_cmds;
        Cmd cmds[MAX_N_CMDS];

        void GetHelpStr(StrBuilder& sb) const {
            sb.AddLine("NAME:");
            sb.AddLine("    %s - %s", name, desc);
            sb.AddLine("");
            sb.AddLine("USAGE:");
            sb.AddLine("    %s command [command options] [arguments...]", name);
            sb.AddLine("");
            sb.AddLine("COMMANDS:");
            for (int i=0; i<n_cmds; ++i) {
                auto& cmd = cmds[i];
                sb.AddLine("    %s - %s", cmd.name, cmd.desc);
            }
        }

        void AddCmd(const Cmd* cmd) {
            if (n_cmds >= MAX_N_CMDS)
                return;

            memcpy(&cmds[n_cmds], cmd, sizeof(Cmd));
            ++n_cmds;
        }


        template<typename A>
        static auto FindCmd(A* app, const char* cmd_name) {
            for (int i=0; i<app->n_cmds; ++i) {
                auto& cmd = app->cmds[i];
                if (!strcmp(cmd.name, cmd_name)) {
                    return &cmd;
                }
            }

            // auto return type requires multiple return clauses must return the exact smae type
            return static_cast<decltype(&app->cmds[0])>(nullptr);
        }

        const Cmd* FindCmd(const char* cmd_name) const {
            return App::FindCmd(this, cmd_name);
        }

        Cmd* FindCmd(const char* cmd_name) {
            return App::FindCmd(this, cmd_name);
        }

        static ActionError HelpAction(void* context, const CmdResult* result, cli::StrBuilder& error) {
            auto app = reinterpret_cast<const App*>(context);
            auto cmd_name = result->GetFirstArg();

            if (!cmd_name) {
                app->PrintHelp();
            } else {
                auto cmd = app->FindCmd(cmd_name);
                if (!cmd) {
                    printerr("Unrecognized command - %s", cmd_name);
                    fprintf(stderr, "\n");
                    app->PrintHelp();
                } else {
                    cmd->PrintHelp(app->name);
                }
            }

            return ACTION_OK;
        }

        void PrintHelp() const {
            StrBuilder sb;
            GetHelpStr(sb);
            fprintf(stderr, "%s\n", sb.buf);
        }

        int Run(int argc, char *argv[]) {
            Cmd help_cmd {
                .name = "help",
                .desc = "Shows a list of commands or help for one command",
                .context = this,
                .action = HelpAction
            };

            AddCmd(&help_cmd);

            if (argc <= 1) {
                PrintHelp();
                return -1;
            }

            auto cmd_name = argv[1];

            auto cmd = FindCmd(cmd_name);
            if (!cmd) {
                printerr("Unrecognized command %s", cmd_name);
                fprintf(stderr, "\n");
                PrintHelp();
                return -1;
            }

            CmdResult result {};
            if (cmd->Parse(argc-2, argv+2, &result)) {
                fprintf(stderr, "\n");
                cmd->PrintHelp(name);
                return -1;
            }

            StrBuilder error;
            auto action_err = cmd->action(cmd->context, &result, error);
            if (action_err != ACTION_OK) {
                if (strlen(error.buf))
                    printerr("%s", error.buf);

                if (action_err == ACTION_WRONG_ARGS) {
                    fprintf(stderr, "\n");
                    cmd->PrintHelp(name);
                }

                return -1;
            }

            return 0;
        }
    };
    
} // namespace Cli


#endif // ANIMTOOL_CLI_H
