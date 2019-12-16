#ifndef TOKENIZER_CPP
#define TOKENIZER_CPP

typedef struct
{
    char *At;
} tokenizer;
tokenizer Tokenizer;

enum token_type
{
    token_Unknown,
    
    token_Pound,      // macros
    token_Dot,        // a field
    token_Comma,      // parameters
    token_Equals,     // assignment
    token_Colon,      // ...?
    token_SemiColon,  // end of statement
    token_BraceOpen,   token_BraceClose,    // various bodies
    token_ParensOpen,  token_ParensClose,   // params, explicit expression order
    token_BracketOpen, token_BracketClose,  // array indexing
    token_ChevronOpen, token_ChevronClose,  // system headers.
    token_Dash,       // ???
    token_Number,     // numbers.
    token_Quote,      // strings
    
    token_Identifier  // ids
};

typedef struct
{
    token_type type;
    
    int length;
    char *text;
} Token;

// Gold
char *ReadAndNullTerminate(char *name)
{
    FILE *f = fopen(name, "rb");
    fseek(f, 0, SEEK_END);
    int filesize = ftell(f);
    
    rewind(f);
    
    char *parsefile = (char *)malloc(sizeof(char) * (filesize + 1));
    fread(parsefile, filesize, 1, f);
    parsefile[filesize] = '\0';
    
    return parsefile;
}

// gold
bool IsLetter(char c)
{
    if(('a' <= c && c <= 'z') ||
       ('A' <= c && c <= 'Z'))
        return true;
    
    return false;
}
bool IsNumber(char c)
{
    if('0' <= c && c <= '9')
        return true;
    return false;
}

bool IsNewline(char c)
{
    if(c == '\n' ||
       c == '\r')
        return true;
    return false;
}
bool IsGarbage(char c)
{
    if(c == ' ' ||
       c == '\t' ||
       IsNewline(c) ||
       c == '\0')// || c == '*')
        return true;
    return false;
}

// identify the token parameters
// if it's an identifier/name/string, gather it
Token GetToken()
{
    while(IsGarbage(Tokenizer.At[0]))
        Tokenizer.At++;
    
    Token token;
    
    token.length = 1;
    token.text = Tokenizer.At;
    
    char c = *Tokenizer.At;
    switch(c)
    {
        case '#': { token.type = token_Pound; } break;
        case '.': { token.type = token_Dot;   } break;
        case ',': { token.type = token_Comma; } break;
        case ':': { token.type = token_Colon;     } break;
        case ';': { token.type = token_SemiColon; } break;
        case '=': { token.type = token_Equals; } break;
        
        case '"': { token.type = token_Quote; } break;
        
        case '{': { token.type = token_BraceOpen;  } break;
        case '}': { token.type = token_BraceClose; } break;
        case '(': { token.type = token_ParensOpen;  } break;
        case ')': { token.type = token_ParensClose; } break;
        case '[': { token.type = token_BracketOpen;  } break;
        case ']': { token.type = token_BracketClose; } break;
        case '<': { token.type = token_ChevronOpen;  } break;
        case '>': { token.type = token_ChevronClose; } break;
        
        default:
        {
            if(IsLetter(c) || (Tokenizer.At[0] == '*' && Tokenizer.At[1] != ' '))
                //if(IsLetter(c))
            {
                token.type = token_Identifier;
                while(IsLetter(Tokenizer.At[0]) ||
                      IsNumber(Tokenizer.At[0]) ||
                      Tokenizer.At[0] == '_' ||
                      Tokenizer.At[0] == '*')
                {
                    Tokenizer.At++;
                }
                token.length = Tokenizer.At - token.text;
                Tokenizer.At--;  // roll back to preserve whatever's after our last letter
            } break; // do we need a break here?
            
            // gosh this is dangerous
            token.type = token_Unknown;
        } break;
    }
    
    return token;
}

// you can also see this as a usage example function...
// normally this would be a main()...
// and I'd have to move everything into an .h
void PrintTokens(char *buffer)
{
    Tokenizer.At = buffer;
    
    // --- Go through the file and break it down into tokens
    while(Tokenizer.At[0])  // advance until terminator
    {
        //char *savepoint = Tokenizer.At;
        Token token;
        
        // get token
        token = GetToken();
        
        // print token
        printf("%.*s\n", token.length, token.text);
        
        // advance tokenizer
        Tokenizer.At++;
        //Tokenizer.At = savepoint + token.length;
    }
}

#endif