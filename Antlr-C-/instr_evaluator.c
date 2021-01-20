
#include "instr_evaluator.h"

int true = 1;
int false = 0;
int error = -1;
void *pNull = NULL;

int *pTrue = &true;
int *pFalse = &false;
int *pError = &error;
void *returnVal = NULL;

void free_data(gpointer data)
{
  g_free(data);
}
/*
  typedef struct ScopesTree
  {
    //fare un puntatore per i figli non è necessario in quanto dal parent non si passa mai ai figli
    struct ScopesTree *parent;

    GHashTable *scope;

    int depth;
  } scope_tree;

  typedef struct ParameterList
  {
    char *type;
    char *name;
    struct ParameterList *next;
  } par_list;

  typedef struct FunctionDefinition
  {
    pANTLR3_BASE_TREE treeBody;
    par_list *parameters;
    char *funName;
    char *func_ret_type;
    GHashTable *scopeValues;
  } fun_def;

  typedef struct Variable
  {
    char *name;
    char *type;
    gpointer value;
    //posizione nella lista
    int position;

    int isPointer;
    char *pointsTo;  // nome della variabile a cui punta
    int pointsToPos; //posizione della variabile
    struct Variable *prev;
    struct Variable *next;

  } var_list;*/

void freeVarList(gpointer listHead)
{
  var_list *head = (var_list *)listHead;
  var_list *tmp;
  //tolgo la ciclicità della lista
  if (head->prev)
  {
    head->prev->next = NULL;
  }
  while (head)
  {

    tmp = head;
    head = head->next;
    //free(tmp->name);
    //free(tmp->type);
    //free(tmp->value);
    tmp->next = NULL;
    tmp->prev = NULL;

    free(tmp);
  }
  head = NULL;
}

/*typedef struct History
  {
    //punto attuale dell'albero sintattico
    pANTLR3_BASE_TREE subTree;
    //lista variabili scritte in quel punto dell'albero sintattico
    struct Variable *written;
    //lista variabili lette in quel punto dell'albero sintattico
    struct Variable *read;
    void *token; //token per stampare la linea cui si riferisce

    struct ScopesTree *scopeTree;

    struct History *prev;
    struct History *next;

  } History;*/

void freeHistory(History **pHistory)
{
  History *freeHistory = *pHistory;
  History *temp;

  while (freeHistory)
  {
    temp = freeHistory;
    temp->subTree = NULL;
    temp->token = NULL;
    if (temp->written)
    {
      freeVarList(temp->written);
      //free(temp->written);
    }
    if (temp->read)
    {
      freeVarList(temp->read);
      //free(temp->read);
    }

    freeHistory = freeHistory->next;

    free(temp);
  }
}

void freeListPar(par_list *head)
{
  par_list *tmp;

  while (head)
  {
    tmp = head;
    head = head->next;
    free(tmp);
  }
  head = NULL;
}

void free_function(gpointer function)
{
  fun_def *func = (fun_def *)function;
  freeListPar(func->parameters);
  if (func->scopeValues)
    g_hash_table_unref(func->scopeValues);
  g_free(func);
}

//da fare
void freeScopeTree(scope_tree *root)
{
}

//hashtable per i valori associati alle variabili
// GHashTable *valuesTable = g_hash_table_new_full(g_str_hash, g_str_equal, free_data, free_data);
//hashtable per le funzioni
//GHashTable *funcTable = g_hash_table_new_full(g_str_hash, g_str_equal, free_data, free_function);
GHashTable *funcTable = NULL;
/*  History *isHistory = malloc(sizeof(History));
  isHistory->subTree = NULL;
  isHistory->written = NULL;
  isHistory->read = NULL;
  isHistory->token = NULL;
  isHistory->prev = NULL;
  isHistory->next = NULL;

  scope_tree *scopeTree = malloc(sizeof(scope_tree));
  scopeTree->parent = NULL;
  scopeTree->scope = valuesTable;
*/
void insertWrittenHistory(History **pHistory, struct Variable *varHistory)
{
  History *history = *pHistory;
  if (!history->written)
  {
    history->written = malloc(sizeof(var_list));
    history->written->name = varHistory->name;
    history->written->type = varHistory->type;
    history->written->position = varHistory->position;
    history->written->next = history->written;
    history->written->prev = history->written;
  }
  else
  { //inserisco in coda
    var_list *tempV = malloc(sizeof(var_list));
    tempV->name = varHistory->name;
    tempV->type = varHistory->type;
    tempV->position = varHistory->position;
    tempV->next = history->written;
    tempV->prev = history->written->prev;
    history->written->prev = tempV;
    tempV->prev->next = tempV;
  }
}

//funzione per inserire chiave e valore nella hashtable scelta, se la chiave e il valore
//sono entrambi char* utilizzare hashInsertChar
void hashInsert(char *keyChar, gpointer value, GHashTable *hashTable)
{
  gchar *key = g_strdup(keyChar);
  if (!hashTable)
  {
    printf("hashtable is null\n");
    return;
  }
  if (!g_hash_table_contains(hashTable, (gconstpointer)key))
  {
    g_hash_table_insert(hashTable, key, value);
  }
  else
  {
    g_hash_table_insert(hashTable, key, value);
  }
}
//globalh non utilizzato da rivedere
void hashInsertVar(char *keyChar, char *type, gpointer value, scope_tree *scopeTree, History **pHistory, int save)
{
  History *history = *pHistory;
  gchar *key = g_strdup(keyChar);
  if (!scopeTree || !scopeTree->scope)
  {
    printf("hashtable is null\n");
    return;
  }
  struct Variable *pVar = NULL;
  if (!g_hash_table_contains(scopeTree->scope, (gconstpointer)key))
  { //primo inserimento della variabile
    pVar = malloc(sizeof(struct Variable));
    pVar->name = key;
    pVar->type = type;
    pVar->position = 1;
    pVar->next = pVar;
    pVar->prev = pVar;
    pVar->isPointer = 0;
    //caso in cui la variabile sia di tipo char, bisogna duplicarla perchè non venga persa
    if (strcmp(type, "char*") == 0 || strcmp(type, "char") == 0)
    {
      char *valueChar = g_strdup(value);
      pVar->value = (gpointer)valueChar;
      g_hash_table_insert(scopeTree->scope, key, pVar);
    }
    else if (strcmp(type, "int") == 0)
    {
      pVar->value = (gpointer)value;
      g_hash_table_insert(scopeTree->scope, key, pVar);
    }
    else if (strcmp(type, "int*") == 0)
    {
      pVar->isPointer = 1; //true
      if (value)
      {
        var_list *varToPointTo = (var_list *)value;
        char *vtptName = varToPointTo->name;
        pVar->pointsTo = vtptName;
        pVar->pointsToPos = varToPointTo->prev->position;
      }
      else
      {
        pVar->pointsTo = NULL;
      }
      g_hash_table_insert(scopeTree->scope, key, pVar);
    }
  }
  else
  {
    //variabile già presente valore da aggiungere
    var_list *list_head = (var_list *)g_hash_table_lookup(scopeTree->scope, (gconstpointer)key);
    if (!list_head)
    {
      printf("value is not found in this scope\n");
      return;
    }
    var_list *temp = list_head;
    while (list_head != temp->next)
    {
      temp = temp->next;
    }
    pVar = malloc(sizeof(var_list));
    pVar->name = key;
    pVar->type = type;
    if (strcmp(type, "char*") == 0 || strcmp(type, "char") == 0)
    {
      char *valueChar = g_strdup(value);
      pVar->value = (gpointer)valueChar;
    }
    else if (strcmp(type, "int") == 0)
    {
      pVar->value = (gpointer)value;
    }
    else if (strcmp(type, "int*") == 0)
    {
      pVar->isPointer = 1; //true

      if (value)
      {
        var_list *varToPointTo = (var_list *)value;
        char *vtptName = varToPointTo->name;
        pVar->pointsTo = vtptName;
        pVar->pointsToPos = varToPointTo->prev->position;
      }
      else
      {
        pVar->pointsTo = NULL;
      }
    }

    temp->next = pVar;
    pVar->prev = temp;
    pVar->next = list_head;
    list_head->prev = pVar;
    pVar->position = temp->position + 1;
  }
  if (save)
    insertWrittenHistory(pHistory, pVar);
}

//funzione che ritorna il valore della chiave riportata nella hashtable
gpointer hashGetVaue(char *key, GHashTable *hashTable, GHashTable *globalHashTable)
{
  GHashTable *scope = hashTable;
  if (!hashTable)
  {
    printf("hashGetVaue: hashtable is null\n");
    return hashTable;
  }
  if (!g_hash_table_contains(hashTable, (gconstpointer)key))
  {
    if (globalHashTable)
    {
      if (g_hash_table_contains(globalHashTable, (gconstpointer)key))
      {
        scope = globalHashTable;
      }
    }
    else
    {
      printf("function %s not found\n", key);
      return NULL;
    }
  }
  gpointer value = g_hash_table_lookup(scope, (gconstpointer)key);
  if (value)
  {
    return value;
  }
  else
  {
    printf("%s 's value is NULL\n", key);
    return NULL;
  }
}

gpointer hashGetVaueVar(char *key, scope_tree *scopeTree, int position, History **pHistory, int save)
{
  History *history = *pHistory;
  GHashTable *scope = scopeTree->scope;
  if (!scopeTree || !scopeTree->scope)
  {
    printf("hashGetVaue: hashtable is null\n");
    return NULL;
  }
  //cerco nell'albero se esiste il valore della chiave
  scope_tree *temp = scopeTree;

  while (!g_hash_table_contains(temp->scope, (gconstpointer)key))
  {
    if (temp->parent)
    {
      temp = temp->parent;

      if (g_hash_table_contains(temp->scope, (gconstpointer)key))
      {
        scope = temp->scope;
      }
    }
    else
    {
      printf("value %s not found\n", key);
      return NULL;
    }
  }

  var_list *valueList = (var_list *)g_hash_table_lookup(scope, (gconstpointer)key);
  if (save)
  {
    if (!history->read)
    {
      var_list *tempV = malloc(sizeof(var_list));
      tempV->name = key;
      tempV->type = valueList->type;
      tempV->position = valueList->prev->position;
      tempV->next = tempV;
      tempV->prev = tempV;
      history->read = tempV;
    }
    else
    { //inserisco il valore in coda
      var_list *tempV = malloc(sizeof(var_list));
      tempV->name = key;
      tempV->type = valueList->type;
      tempV->position = valueList->prev->position;
      tempV->next = history->read;
      tempV->prev = history->read->prev;
      history->read->prev = tempV;
      tempV->prev->next = tempV;
    }
  }
  return valueList;
}

//stampa il la riga da cui viene il token e il suo numero nel file
void printLine(pANTLR3_COMMON_TOKEN token)
{
  if (token != NULL)
  {
    char *line = (char *)token->lineStart;
    int i = 0;
    int lineNumber = (int)token->line;
    printf("line %d: ", lineNumber);
    if (line)
    {
      while (line[i] != ';' && line[i] != '{')
      {
        if (line[i] == '\t')
        {
          continue;
        }
        if (line[i])
        {
          printf("%c", line[i]);
        }
        i = i + 1;
      }
    }
    printf("\n\n");
  }
}

//funzione che richiede l'input dell'utente per stampare variabili, andare avanti nelle istruzioni passo passo
void inputString(scope_tree *scope, History **pHistory)
{
  char userInput1[50];
  char *pUserInput1 = userInput1;
  char userInput2[25];
  char *pUserInput2 = userInput2;
  char userInput3[25];
  char *pUserInput3 = userInput3;
  while (strcmp(pUserInput2, "n") != 0)
  {
    printf("enter a command (h for commands)\n");
    fgets(userInput1, 49, stdin);
    sscanf(userInput1, "%s %s", pUserInput2, pUserInput3);

    if (strcmp(pUserInput2, "n") == 0)
    {
      //do nothing, continue with the istructions

      History *history = *pHistory;
      if (history && history->next)
      {
        while (history && history->next)
        {
          //printf("--------redoing historyy--------\n");
          evaluate(history->next->subTree, &history->next->scopeTree, pHistory, 1);
          //printf("--------historyy redone--------\n");

          history = *pHistory;
        }
      }
      else
        printf("next line:\n");
    }
    else if (strcmp(pUserInput2, "prev") == 0)
    {
    }
    else if (strcmp(pUserInput2, "h") == 0)
    {
      printf("'n' for next instruction\n");
      printf("'print'+ 'var name' to print variable name and type\n\n");
    }
    else if (strcmp(pUserInput2, "p") == 0)
    {
      History *history = *pHistory;
      if (!history)
      {
        printf("no instructions to rewind\n");
        return;
      }
      var_list *temp = history->written;
      if (temp)
      {
        do
        {
          char *varWritten = temp->name;
          printf("varWritten: %s\n", varWritten);

          var_list *varList = (var_list *)hashGetVaueVar(varWritten, history->scopeTree, -1, pHistory, false);
          if (varList)
          {
            //rimuovo l'ultima occorrenza della variabile
            var_list *toFree = varList->prev;
            toFree->prev->next = varList;
            toFree->next->prev = toFree->prev;
            free(toFree);
          }
          temp = temp->next;

        } while (temp != history->written);
      }
      if (history->prev)
      {
        *pHistory = history->prev;
        history = *pHistory;
      }

      printLine(history->subTree->getToken(history->subTree));
      printf("input p\n");
    }
    //da togliere
    else if (pUserInput1[0] == 's' && userInput1[1] == ' ')
    {
      printf("userinput +2: %s\n", userInput1 + 2);
    }
    else if (strcmp(pUserInput2, "print") == 0)
    {
      char *varType;
      if (userInput3[0] != '\0')
      {
        var_list *varList = (var_list *)hashGetVaueVar(userInput3, scope, -1, pHistory, false);
        if (varList)
        {
          varType = (char *)varList->type;
          //printf("variabel type in inputstring: %s\n", varType);
          //printf("variabel value in inputstring: %s\n", (char *)varList->prev->value);
          if (strcmp(varType, "char") == 0 || strcmp(varType, "char*") == 0)
          {
            char *var = (char *)varList->prev->value;
            if (!var)
            {
              printf("value not found\n");
            }
            printf("\nvariable %s:\n type: %s\n value %s\n\n", userInput3, varType, var);
          }
          else if (strcmp(varType, "int") == 0)
          {
            int *var = (int *)varList->prev->value;
            if (!var)
              printf("value not found\n");
            if (varType && var)
              printf("\nvariable %s:\n type: %s\n value %d\n\n", userInput3, varType, *var);
          }
          else if (strcmp(varType, "int*") == 0)
          {
            char *varPointingTo = varList->prev->pointsTo;
            int varPToPos;
            int *valuePointed;
            if (varPointingTo)
            {
              varPToPos = varList->prev->pointsToPos;
              var_list *varPointed = (var_list *)hashGetVaueVar(varPointingTo, scope, -1, pHistory, false);
              valuePointed = (int *)varPointed->prev->value;
              printf(" int pointer pointing to variable: %s\n  %s = %d\n", varPointingTo, varPointingTo, *valuePointed);
            }
            else
            {
              valuePointed = pNull;
              printf(" int pointer pointing to nothing \n");
            }
          }
        }
      }
    }
    else
    {
      printf("comando non riconosciuto\n");
    }
  }
}

// funzione che attraversa l'albero ed esegue le funzioni associate al tipo char
char *charEvaluator(pANTLR3_BASE_TREE tree, scope_tree *scope, History **pHistory)
{
  pANTLR3_COMMON_TOKEN tok = tree->getToken(tree);
  if (tok)
  {
    switch (tok->type)
    {
    case ID:
    {
      char *var = tree->getText(tree)->chars;
      //check type
      //int value = atoi(hashGetVaue(var, valuesTable));
      var_list *varL = (var_list *)hashGetVaueVar(var, scope, -1, pHistory, true);
      char *value = (char *)varL->type;

      return value;
    }
    case CHARACTER_LITERAL:
    {
      return tree->getText(tree)->chars;
    }
    case STRING_LITERAL:
    {
      return tree->getText(tree)->chars;
    }
    }
  }
}

// funzione che attraversa l'albero ed esegue le funzioni associate al tipo int
int numberEvaluator(pANTLR3_BASE_TREE tree, scope_tree *scope, History **pHistory)
{

  pANTLR3_COMMON_TOKEN tok = tree->getToken(tree);
  if (!scope || !scope->scope)
  {
    printf("scope in number evaluator is null");
  }
  if (tok)
  {
    switch (tok->type)
    {
    case INT:
    {
      char *s = tree->getText(tree)->chars;
      int value = atoi(s);
      return value;
    }
    case ID:
    {
      char *var = tree->getText(tree)->chars;
      var_list *varL = (var_list *)hashGetVaueVar(var, scope, -1, pHistory, true);
      if (!varL)
      {
        printf("%s not found in scope\n", var);
      }
      int *value = (int *)varL->prev->value;
      if (!value)
      {
        printf("%s value is null or not initializedd\n", var);
        //return 0;
      }
      return *value;
    }
    case UN_OP:
    {
      //da fare caso con '*' dereferenziazione
      pANTLR3_BASE_TREE unop = (pANTLR3_BASE_TREE)tree->getChild(tree, 0); //controllo se si tratta di & o *
      if (strcmp(unop->getText(unop)->chars, "*") != 0)
      {
        printf("wrong type assigned to int at line %d/n", *(int *)unop->getToken(unop)->getLine);
      }
      pANTLR3_BASE_TREE rhs_id_tree = (pANTLR3_BASE_TREE)tree->getChild(tree, 1); //controllo se si tratta di & o *
      char *rhs_id = rhs_id_tree->getText(rhs_id_tree)->chars;
      var_list *varL = (var_list *)hashGetVaueVar(rhs_id, scope, -1, pHistory, true);
      if (!varL->isPointer)
      {
        printf("dereferencing a non pointer variable: %s\n", rhs_id); //da rivedere con char*
        return 0;
      }
      char *rhs_points_to = varL->prev->pointsTo;
      var_list *varPointedTo = (var_list *)hashGetVaueVar(rhs_points_to, scope, -1, pHistory, true);

      int *value = (int *)varPointedTo->prev->value;
      if (!value)
      {
        printf("%s value is null or not initializedu\n", rhs_id);
        //return 0;
      }
      return *value;
    }
    case PLUS:
    {
      return numberEvaluator(tree->getChild(tree, 0), scope, pHistory) + numberEvaluator(tree->getChild(tree, 1), scope, pHistory);
    }
    case MINUS:
    {
      return numberEvaluator(tree->getChild(tree, 0), scope, pHistory) - numberEvaluator(tree->getChild(tree, 1), scope, pHistory);
    }
    case MULT:
    {
      return numberEvaluator(tree->getChild(tree, 0), scope, pHistory) * numberEvaluator(tree->getChild(tree, 1), scope, pHistory);
    }
    case DIVI:
    {
      return numberEvaluator(tree->getChild(tree, 0), scope, pHistory) / numberEvaluator(tree->getChild(tree, 1), scope, pHistory);
    }
    case REST:
    {
      return numberEvaluator(tree->getChild(tree, 0), scope, pHistory) % numberEvaluator(tree->getChild(tree, 1), scope, pHistory);
    }
    }
  }
}

//funzione ausiliaria che aggiunge un oggetto par_list alla fine di una lista "head"
void addEnd(par_list **head, char *type, char *name)
{

  par_list *newPar = (par_list *)malloc(sizeof(par_list));
  newPar->type = type;
  newPar->name = name;

  if (!*head)
  {
    *head = newPar;
    return;
  }
  par_list *temp = *head;
  while (temp->next)
  {
    temp = temp->next;
  }
  temp->next = newPar;

  return;
}

//funzione principale che attraversa l'albero e gestisce ogni azione
int returnValue;

void *evaluate(pANTLR3_BASE_TREE tree, scope_tree **pScope, History **pHistory, int saveHistory)
{
  scope_tree *scope = *pScope;
  pANTLR3_COMMON_TOKEN tok = tree->getToken(tree);
  if (!funcTable)
    funcTable = g_hash_table_new_full(g_str_hash, g_str_equal, free_data, free_function);

  if (tok)
  {
    History *history = *pHistory;
    if (saveHistory)
    {
      //istanzio history per salvare le azioni svolte
      if (!history)
      {
        history = malloc(sizeof(History));
        history->subTree = tree;
        history->written = NULL;
        history->read = NULL;
        history->prev = NULL;
        history->next = NULL;
        history->scopeTree = scope;
        *pHistory = history;
      }
      else
      {
        if (!history->next)
        { //se esiste history->next vuol dire che sono tornato indietro (p) e sto tornando avanti (n)
          History *tempH = malloc(sizeof(History));
          tempH->subTree = tree;
          tempH->written = NULL;
          tempH->read = NULL;
          tempH->prev = history;
          tempH->next = NULL;
          tempH->scopeTree = *pScope;

          history->next = tempH;
        }
        else
        { //elimino gli eventuali salvataggi fatti da history precedentemente
          if (history->next->written)
          {
            freeVarList(history->next->written);
            //free(history->next->written);
            history->next->written = NULL;
          }
          if (history->next->written)
          {
            freeVarList(history->next->read);
            //free(history->next->read);
            history->next->read = NULL;
          }
        }
        *pHistory = history->next;
      }
    }
    switch (tok->type)
    {
    case INT:
    {
      //printf("token data int: %s\n", &tok->input->nextChar);
      //const char *s = tree->getText(tree)->chars;
      int value = numberEvaluator(tree, scope, pHistory);
      returnVal = &value;
      return returnVal;
    }

    case CHARACTER_LITERAL:
    {
      return tree->getText(tree)->chars;
    }
    case STRING_LITERAL:
    {
      return tree->getText(tree)->chars;
    }
    case ID:
    {
      char *var = tree->getText(tree)->chars;
      //check type
      var_list *varL = (var_list *)hashGetVaueVar(var, scope, -1, pHistory, true);
      if (!varL)
      {
        printf("no values for %s found\n", var);
      }

      return varL->prev->value;
    }
    case UN_OP:
    {
      //dcontrollo '*' dereferenziazione
      pANTLR3_BASE_TREE unop = (pANTLR3_BASE_TREE)tree->getChild(tree, 0); //controllo se si tratta di & o *
      if (strcmp(unop->getText(unop)->chars, "*") != 0)
      {
      }
      pANTLR3_BASE_TREE rhs_id_tree = (pANTLR3_BASE_TREE)tree->getChild(tree, 1); //controllo se si tratta di & o *
      char *rhs_id = rhs_id_tree->getText(rhs_id_tree)->chars;
      var_list *varL = (var_list *)hashGetVaueVar(rhs_id, scope, -1, pHistory, true);
      if (!varL->isPointer)
      {
        printf("dereferencing a non pointer variable: %s\n", rhs_id); //da rivedere con char*
        return 0;
      }
      char *rhs_points_to = varL->prev->pointsTo;
      var_list *varPointedTo = (var_list *)hashGetVaueVar(rhs_points_to, scope, -1, pHistory, true);

      return varPointedTo;
    }
    case EQEQ:
    {
      int firstValue = (int)numberEvaluator(tree->getChild(tree, 0), scope, pHistory);
      int secondValue = (int)numberEvaluator(tree->getChild(tree, 1), scope, pHistory);
      printLine(tok);
      inputString(scope, pHistory);
      if (firstValue == secondValue)
      {
        return pTrue;
      }
      else
      {
        return NULL;
      }
    }
    case NEQ:
    {
      int firstValue = (int)numberEvaluator(tree->getChild(tree, 0), scope, pHistory);
      int secondValue = (int)numberEvaluator(tree->getChild(tree, 1), scope, pHistory);
      printLine(tok);
      inputString(scope, pHistory);
      if (firstValue != secondValue)
      {
        return pTrue;
      }
      else
      {
        return NULL;
      }
    }
    case OPLT:
    {
      int firstValue = numberEvaluator(tree->getChild(tree, 0), scope, pHistory);
      int secondValue = numberEvaluator(tree->getChild(tree, 1), scope, pHistory);
      printLine(tok);
      inputString(scope, pHistory);
      if (firstValue < secondValue)
      {
        return pTrue;
      }
      else
      {
        return pFalse;
      }
    }
    case PLUS:
    {
      returnValue = numberEvaluator(tree, scope, pHistory);
      return &returnValue;
    }
    case MINUS:
    {
      returnValue = numberEvaluator(tree, scope, pHistory);
      return &returnValue;
    }
    case MULT:
    {
      returnValue = numberEvaluator(tree, scope, pHistory);
      return &returnValue;
    }
    case DIVI:
    {
      returnValue = numberEvaluator(tree, scope, pHistory);
      return &returnValue;
    }
    case REST:
    {
      returnValue = numberEvaluator(tree, scope, pHistory);
      return &returnValue;
    }
    case VAR_DEF:
    {
      //metti la variabile nella tabella hash con valore null
      pANTLR3_BASE_TREE child0 = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
      char *variableType = child0->getText(child0)->chars;
      pANTLR3_BASE_TREE child1 = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
      char *variableName = child1->getText(child1)->chars;
      hashInsertVar(variableName, variableType, pNull, scope, pHistory, true);
      printLine(child0->getToken(child0));
      inputString(scope, pHistory);
      return pTrue;
    }
    case POI_DEF:
    {
      //puntatore e tipo nelle rispettive tabelle hash
      pANTLR3_BASE_TREE child0 = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
      pANTLR3_BASE_TREE child00 = (pANTLR3_BASE_TREE)child0->getChild(child0, 0);
      char *variableType = child00->getText(child00)->chars;
      pANTLR3_BASE_TREE child1 = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
      char *variableName = child1->getText(child1)->chars;
      char asterisk = '*';
      strncat(variableType, &asterisk, 1);
      printf("variable type = %s\n", variableType);
      hashInsertVar(variableName, variableType, pNull, scope, pHistory, true);
      printLine(child00->getToken(child00));
      inputString(scope, pHistory);
      return pTrue;
    }
    case EQ:
    {
      int i = 0;
      //caso in cui si faccia definizione e assegnamento insieme
      if (tree->getChildCount(tree) == 3)
      {
        pANTLR3_BASE_TREE child0 = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
        char *variableType;
        //caso di una variabile normale
        if (child0->getChildCount(child0) == 0)
        {
          variableType = child0->getText(child0)->chars;
        }
        //caso di un puntatore
        else if (child0->getChildCount(child0) == 1)
        {
          child0 = child0->getChild(child0, 0);
          variableType = child0->getText(child0)->chars;
          char asterisk = '*';
          strncat(variableType, &asterisk, 1);
        }
        pANTLR3_BASE_TREE child1 = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
        char *variableName = child1->getText(child1)->chars;

        hashInsertVar(variableName, variableType, pNull, scope, pHistory, false); // da non inserire come written
        i = i + 1;
      }

      //caso di assegnamento
      pANTLR3_BASE_TREE child00 = (pANTLR3_BASE_TREE)tree->getChild(tree, i);
      char *variableName = child00->getText(child00)->chars;
      var_list *varL = (var_list *)hashGetVaueVar(variableName, scope, -1, pHistory, false);

      char *type = (char *)varL->type;
      //string var(getText(getChild(tree, 0)));
      if (!type)
      {
        printf("error, no type found for variable\n");
        return pError;
      }
      if (strcmp(type, "int") == 0)
      {
        //da rivedere assegnamento tramite funzione per i tipi non int

        pANTLR3_BASE_TREE child1 = (pANTLR3_BASE_TREE)tree->getChild(tree, i + 1);
        //caso di assegnamento tramite una funzione
        int value;
        if (child1->getToken(child1)->type == FUNC_CALL)
        {
          pANTLR3_BASE_TREE funNameTree = (pANTLR3_BASE_TREE)child1->getChild(child1, 0);
          char *funName = funNameTree->getText(funNameTree)->chars;

          fun_def *function = hashGetVaue(funName, funcTable, NULL);
          if (!function)
          {
            printf("function %s not found\n", funName);
            return pError;
          }
          char *fReturnType = function->func_ret_type;
          if (strcmp(fReturnType, "int") == 0)
          {
            value = *(int *)evaluate(child1, pScope, pHistory, 0);
            printf("func eq value: %d\n", value);
          }
          else if (strcmp(fReturnType, "int*") == 0)
          {
            int *pValue = (int *)evaluate(child1, pScope, pHistory, 0);
          }
          else if (strcmp(fReturnType, "char*") || strcmp(fReturnType, "char"))
          {
            char *pValue = (char *)evaluate(child1, pScope, pHistory, 0);
          }
        }
        else
        {
          value = numberEvaluator(child1, scope, pHistory);
        }
        char strVal[12];
        int *pValue = malloc(sizeof(int));
        *pValue = value;
        //printf("equation right value : %d\n", *pValue);
        //sprintf(strVal, "%d", value);
        hashInsertVar(variableName, type, pValue, scope, pHistory, true);
        printLine(tok);
        inputString(scope, pHistory);
        return pValue;
      }

      else if (strcmp(type, "char") == 0)
      {
        pANTLR3_BASE_TREE child1 = (pANTLR3_BASE_TREE)tree->getChild(tree, i + 1);
        char *value = charEvaluator(child1, scope, pHistory);
        char *pValue;
        pValue = malloc(sizeof(char));
        printf("value[1]:%c", value[1]);
        *pValue = value[1];
        hashInsertVar(variableName, type, pValue, scope, pHistory, true);
        printLine(tok);
        inputString(scope, pHistory);
        return pValue;
      }
      else if (strcmp(type, "char*") == 0)
      {
        pANTLR3_BASE_TREE child1 = (pANTLR3_BASE_TREE)tree->getChild(tree, i + 1);
        char *value = charEvaluator(child1, scope, pHistory);
        int valueSize = strlen(value);
        value[valueSize - 1] = '\0';
        value = value + 1;

        gpointer pValue = g_strdup(value);
        hashInsertVar(variableName, type, pValue, scope, pHistory, true);
        printLine(tok);
        inputString(scope, pHistory);
        return pValue;
      }

      else if (strcmp(type, "int*") == 0)
      {
        //si aspetta un puntatore come rhs
        var_list *pValue;
        pValue = malloc(sizeof(var_list));
        pANTLR3_BASE_TREE child1 = (pANTLR3_BASE_TREE)tree->getChild(tree, i + 1); // right hand side
        //printf("child1 %s\n", child1->toStringTree(child1)->chars);
        //o idp o &variabile o puntatore alla variabile
        //1)
        //da rivedere
        if (child1->getToken(child1)->type == FUNC_CALL)
        {
          pANTLR3_BASE_TREE funNameTree = (pANTLR3_BASE_TREE)child1->getChild(child1, 0);
          char *funName = funNameTree->getText(funNameTree)->chars;

          fun_def *function = hashGetVaue(funName, funcTable, NULL);
          if (!function)
          {
            printf("function %s not found\n", funName);
            return pError;
          }
          var_list *pValue = (var_list *)evaluate(child1, pScope, pHistory, 0);
          //printf("test pValue int* func in line 1103 %s\n", pValue->name);
          hashInsertVar(variableName, type, pValue, scope, pHistory, true); //ispointer
        }

        else if (child1->getToken(child1)->type == UN_OP)
        {
          pANTLR3_BASE_TREE unop = (pANTLR3_BASE_TREE)child1->getChild(child1, 0); //controllo se si tratta di & o *
          if (strcmp(unop->getText(unop)->chars, "&") != 0)
          {
            printf("wrong type assigned to pointer %s\n", variableName);
          }
          pANTLR3_BASE_TREE rhs_id_tree = (pANTLR3_BASE_TREE)child1->getChild(child1, 1); //controllo se si tratta di & o *
          char *rhs_id = rhs_id_tree->getText(rhs_id_tree)->chars;
          *pValue = *(var_list *)hashGetVaueVar(rhs_id, scope, -1, pHistory, true);
          //printf("test pValue int* unop %s\n", pValue->name);
          //printf("pvalue unop; %d\n", *(int *)pValue->prev->value);
          hashInsertVar(variableName, type, pValue, scope, pHistory, true); //ispointer
        }

        else if (child1->getToken(child1)->type == ID)
        {
          char *rhs_id = child1->getText(child1)->chars;
          //prendo la variabile a cui punta il puntatore a destra dell'uguale
          var_list *pointerValue = (var_list *)hashGetVaueVar(rhs_id, scope, -1, pHistory, true);
          *pValue = *(var_list *)hashGetVaueVar(pointerValue->prev->pointsTo, scope, -1, pHistory, true);
          printf("test pValue int* id %s\n", pValue->prev->pointsTo);
          //lo inserisco come oggetto puntato dal puntatore a sinistra dell'uguale
          hashInsertVar(variableName, type, pValue, scope, pHistory, true); //ispointer
        }

        printLine(tok);
        inputString(scope, pHistory);
        return pValue;
      }
      else
      {
        printf("type not found\n");
        return pError;
      }
    }
    case IF_STAT:
    {
      //cancello l'ultima occorrenza di history in quanto si tratta di un token immaginario
      History *histDel = *pHistory;
      histDel->prev->next = NULL;
      *pHistory = histDel->prev;
      free(histDel);
      //caso if senza else
      pANTLR3_BASE_TREE childIFELSE = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
      pANTLR3_BASE_TREE childIF = (pANTLR3_BASE_TREE)childIFELSE->getChild(childIFELSE, 0);

      int *ifClauseValue = (int *)evaluate(childIF, pScope, pHistory, 1);

      //condizione dell'if è vera quindi svolgo il codice nell'if
      if (ifClauseValue)
      {
        printf("->if\n");
        return evaluate((pANTLR3_BASE_TREE)childIFELSE->getChild(childIFELSE, 1), pScope, pHistory, 0);
      }
      else if (tree->getChildCount(tree) > 1)
      {
        printf("->else\n");
        pANTLR3_BASE_TREE childELSEIF = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
        pANTLR3_BASE_TREE childELSE = (pANTLR3_BASE_TREE)childELSEIF->getChild(childELSEIF, 0);
        return evaluate(childELSE, pScope, pHistory, 0);
      }
    }
    case WHI_STAT:
    {
      //cancello l'ultima occorrenza di history in quanto si tratta di un token immaginario
      History *histDel = *pHistory;
      histDel->prev->next = NULL;
      *pHistory = histDel->prev;
      free(histDel);

      pANTLR3_BASE_TREE conditionalExpr = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
      int *conditionalExprEval = (int *)evaluate(conditionalExpr, pScope, pHistory, 1);

      while (conditionalExprEval)
      {
        evaluate((pANTLR3_BASE_TREE)tree->getChild(tree, 1), pScope, pHistory, 0);
        conditionalExprEval = evaluate(conditionalExpr, pScope, pHistory, 1);
      }
      return conditionalExprEval;
    }
    case FUNC_DEF:
    {
      //funcTable come variable globale(da passa recome parametro se si sposta)
      //cancello history in quanto non è necessario rifare questa parte
      History *histDel = *pHistory;
      histDel->prev->next = NULL;
      *pHistory = histDel->prev;
      free(histDel);

      pANTLR3_BASE_TREE header = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
      pANTLR3_BASE_TREE body = (pANTLR3_BASE_TREE)tree->getChild(tree, 1);
      pANTLR3_BASE_TREE funType = (pANTLR3_BASE_TREE)header->getChild(header, 0);
      char *funTypeChar = funType->getText(funType)->chars;
      pANTLR3_BASE_TREE funName = (pANTLR3_BASE_TREE)header->getChild(header, 1);

      if (!g_hash_table_contains(funcTable, (gconstpointer)funName))
      {

        fun_def *pFun = g_malloc0(sizeof(fun_def));

        pFun->func_ret_type = funTypeChar;
        pFun->treeBody = body;

        //nome della funzione
        char *funNameChar = funName->getText(funName)->chars;
        pFun->funName = funNameChar;

        int k = header->getChildCount(header);
        printf("number of parameters: %d \n", k - 2);

        par_list *parameter_list_head = NULL;
        for (int i = 2; i < k; i++)
        {
          pANTLR3_BASE_TREE paramTree = (pANTLR3_BASE_TREE)header->getChild(header, i);

          pANTLR3_BASE_TREE parType = (pANTLR3_BASE_TREE)paramTree->getChild(paramTree, 0);
          pANTLR3_BASE_TREE parName = (pANTLR3_BASE_TREE)paramTree->getChild(paramTree, 1);
          //parameter->type = partype->getText(parType)->chars;
          //parameter->name = parname->getText(parname)->chars;

          char *parTypeChar = parType->getText(parType)->chars;
          char *parNameChar = parName->getText(parName)->chars;

          addEnd(&parameter_list_head, parTypeChar, parNameChar);
        }
        pFun->parameters = parameter_list_head;
        hashInsert(funNameChar, pFun, funcTable);
        return pTrue;
      }
      else
        return pFalse;
    }
    case FUNC_CALL:
    {
      pANTLR3_BASE_TREE fName = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
      char *fNameChar = fName->getText(fName)->chars;
      int nParams = tree->getChildCount(tree);
      fun_def *function = (fun_def *)hashGetVaue(fNameChar, funcTable, NULL);
      char *fReturnType = function->func_ret_type;
      if (!function)
      {
        printf("function %s not found\n", fNameChar);
        return 0;
      }

      function->scopeValues = g_hash_table_new_full(g_str_hash, g_str_equal, free_data, freeVarList);
      par_list *temp = function->parameters;
      char *paramType;
      char *paramName;
      //inserimento nell'albero degli scope
      scope_tree *scopeChild = malloc(sizeof(scope_tree));
      scopeChild->parent = *pScope;
      scopeChild->scope = function->scopeValues;
      History *tempH = *pHistory;
      tempH->scopeTree = scopeChild;

      for (int i = 1; i < nParams; i++)
      {
        /* metti i parametri nello scope */
        paramType = temp->type;
        paramName = temp->name;
        if (strcmp(paramType, "int") == 0)
        {
          int *paramValue = malloc(sizeof(int));
          pANTLR3_BASE_TREE paramValueTree = (pANTLR3_BASE_TREE)tree->getChild(tree, i);
          *paramValue = numberEvaluator(paramValueTree, scopeChild, pHistory);
          printf("paramtype: %s ", paramType);
          printf("paramValue: %d\n", *paramValue);
          hashInsertVar(paramName, paramType, paramValue, scopeChild, pHistory, false);
        }
        else if (strcmp(paramType, "int*") == 0)
        {
          int *paramValue = malloc(sizeof(int));
          pANTLR3_BASE_TREE paramValueTree = (pANTLR3_BASE_TREE)tree->getChild(tree, i);
          paramValue = evaluate(paramValueTree, pScope, pHistory, 1); //da rivedere -> evaluatepointer
          printf("paramtype: %s ", paramType);
          printf("paramValue: %d\n", *paramValue);
          hashInsertVar(paramName, paramType, paramValue, scopeChild, pHistory, false);
        }
        else if (strcmp(paramType, "char") == 0 || strcmp(paramType, "char*") == 0)
        {
          printf("paramtype: %s ", paramType);
          char *paramValue = charEvaluator((pANTLR3_BASE_TREE)tree->getChild(tree, i), scope, pHistory);
          printf("paramValue: %d\n", *paramValue);
          hashInsertVar(paramName, paramType, paramValue, scopeChild, pHistory, false);
        }
        temp = temp->next;
      }
      printLine(fName->getToken(fName));
      inputString(scope, pHistory);
      if (strcmp(fReturnType, "int") == 0 || strcmp(fReturnType, "int*") == 0)
        return (int *)evaluate(function->treeBody, &scopeChild, pHistory, 0);
      if (strcmp(fReturnType, "char") == 0 || strcmp(fReturnType, "char*") == 0)
        return (char *)evaluate(function->treeBody, &scopeChild, pHistory, 0);
    }
    case RET:
    {
      pANTLR3_BASE_TREE expr = (pANTLR3_BASE_TREE)tree->getChild(tree, 0);
      gpointer retValue = evaluate(expr, pScope, pHistory, 0);

      printLine(expr->getToken(expr));
      inputString(scope, pHistory);
      return retValue;
    }
    case BLOCK:
    {
      //devo togliere l'ultimo blocco history dato che conterrebbe tutto il blocco e non le singole istruzioni
      /*History *temp = *pHistory;
        *pHistory = temp->prev;
        temp->prev->next = NULL;
        free(temp);
        */
      int k = tree->getChildCount(tree);
      int *r = NULL;
      for (int i = 0; i < k; i++)
      {
        /*History *history = *pHistory;
        while (history && history->next)
        {
          r = evaluate(history->next->subTree, &history->next->scopeTree, pHistory, 1);
          printf("--------redoing historyy--------\n");
          history = *pHistory;
        }*/
        r = evaluate(tree->getChild(tree, i), pScope, pHistory, 1);
      }
      return r;
    }
    default:
      printf("Unhandled token: %d\n", tok->type);
      return pError;
    }
  }
  else
  {
    int k = tree->getChildCount(tree);
    int *r = NULL;
    for (int i = 0; i < k; i++)
    {
      /*
      while (history && history->next)
      {
        r = evaluate(history->next->subTree, &history->next->scopeTree, pHistory, 1);
        printf("--------redoing historyy--------\n");
        history = *pHistory;
      }*/
      pANTLR3_BASE_TREE child = tree->getChild(tree, i);
      r = evaluate(child, pScope, pHistory, 1);

      History *history = *pHistory;
      /*if (history && history->read)
      {
        History *tempwhiler = history;
        do
        {
          if (tempwhiler->read)
          {
            printf("variables read %s, ", tempwhiler->read->name);
            var_list *tempvarlist = tempwhiler->read;
            while (tempvarlist->next != tempwhiler->read)
            {
              tempvarlist = tempvarlist->next;
              printf("%s", tempvarlist->name);
            }
            printf("\n");
          }
          tempwhiler = tempwhiler->prev;
        } while (tempwhiler);
        printf("\n");
      }
      if (history && history->written)
      {
        History *tempwhilew = history;
        do
        {
          if (tempwhilew->written)
          {
            printf("variables written %s, ", tempwhilew->written->name);
            var_list *tempvarlistw = tempwhilew->written;
            while (tempvarlistw->next != tempwhilew->written)
            {
              tempvarlistw = tempvarlistw->next;
              printf("%s", tempvarlistw->name);
            }
            printf("\n");
          }
          tempwhilew = tempwhilew->prev;
        } while (tempwhilew);
        printf("\n");
      }*/
      /*if (child->getToken(child)->type == RET)
        {
          break;
        }*/
    }
    return r;
  }
}
