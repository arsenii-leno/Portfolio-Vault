#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdint.h>

#define LINE_MAX 1024
#define MAX_LINE_LEN 256

/* ===========================================================
   Project: Terminal-based CRM System in C
   Description: Wall Street Grade Client & Transaction Manager
   Language: C17 \\ language is C but version is 17 why are you pasting it here?
   =========================================================== */

typedef struct Transaction {
    char transaction_id[37];   
    char client_id[37];        
    double amount;           
    char currency[4];          
    char payment_method[20];   
    char status[15];           
    long long timestamp;         

    struct Transaction *next;  
} TRANSACTION;

typedef struct Client {
    char client_id[37];        
    char full_name[100];       
    char email[100];           
    char phone_number[20];     
    char country_code[3];      
    
    double lifetime_value;     
    long long registration_date; 

    struct Client *next;              
    struct Transaction *transactions; 
} CLIENT;

/* ===========================================================
   Memory Management & Helpers
   =========================================================== */

void free_transactions(TRANSACTION *head) {
    while(head) {
        TRANSACTION *tmp = head;
        head = head->next;
        free(tmp);
    }
}

void free_clients(CLIENT **head) {
    CLIENT *curr = *head;
    while(curr) {
        CLIENT *tmp = curr;
        curr = curr->next;
        free_transactions(tmp->transactions);
        free(tmp);
    }
    *head = NULL;
}

void free_rows(char ***array, int size) {
    if (!array || !*array) return;
    for(int i = 0; i < size; i++){
        free((*array)[i]);
    }
    free(*array);
    *array = NULL;
}

void get_field_value(const char *line, int index, char *buffer) {
    const char *start = line;
    int current_col = 0;
    buffer[0] = '\0'; 

    while (*start) {
        if (current_col == index) {
            char *end = strchr(start, '#');
            if (end) {
                int len = end - start; 

                strncpy(buffer, start, len);
                buffer[len] = '\0';
            } else {
                strcpy(buffer, start);
            }
            return;
        }
        start = strchr(start, '#');
        if (!start) break;
        start++;
        current_col++;
    }
}

int array_manage(char *str, char *parts[], int max_parts) {
    int count = 0;
    char *p = str;
    char *hash;

    while (count < max_parts) {
        parts[count++] = p;

        hash = strchr(p, '#'); 
        if (!hash) break;
        *hash = '\0';
        p = hash + 1;
    }
    return count;
}

int check_client(char *name, char *year, CLIENT *client){
    return strcmp(name, client->full_name) == 0 && client->registration_date == atoll(year);
}

/* ===========================================================
   Data Operations
   =========================================================== */

int push(char ***transactions, int *count, int id, const char *line) {
    char **tmp, **arr;

    tmp = (char**)realloc(*transactions, (*count + 1) * sizeof(char *));
    if (!tmp) {
        printf("System: Memory allocation failed during push.\n");
        return 0;
    }

    *transactions = tmp;
    arr = *transactions;

    for (int i = *count; i > id; i--) {
        arr[i] = arr[i - 1];
    }
    arr[id] = (char*)malloc(strlen(line) + 1);
    if (!arr[id]) {
        printf("System: Memory allocation failed for row.\n");
        return 0;
    }
    strcpy(arr[id], line);
    (*count)++;

    return 1;
}

int pop(char ***transactions, int *transactions_count, const char *tid) {
    int removed = 0;
    char **tmp;

    for(int i = 0; i < *transactions_count; i++) {
        if(strstr((*transactions)[i], tid) != NULL) {
            free((*transactions)[i]); 
            removed++;

            for(int j = i; j < *transactions_count - 1; j++) {
                (*transactions)[j] = (*transactions)[j+1]; 
            }
            (*transactions_count)--;
            i--;
        }
    }
    
    if (*transactions_count > 0) {
        tmp = (char **)realloc(*transactions, (*transactions_count) * sizeof(char*));
        if (tmp) {
            *transactions = tmp;
        } else {
            free(*transactions);
            *transactions = NULL;
        }
    }
    return removed;
}

void scan_data(char *tx_id, char *c_id, double *amount, char *currency, char *pay, char *status, long long *stamp) {
    printf("Enter TX_ID: "); 
    scanf("%36s", tx_id);

    printf("Enter C_ID: "); 
    scanf("%36s", c_id);

    printf("Enter Amount: "); 
    scanf("%lf", amount);

    printf("Enter Currency: "); 
    scanf("%3s", currency);

    printf("Enter Payment Method: "); 
    scanf("%19s", pay);

    printf("Enter Status: "); 
    scanf("%14s", status);

    printf("Enter Timestamp (Unix): "); 
    scanf("%lld", stamp);
}

/* ===========================================================
   Core CRM Functions
   =========================================================== */

void cmd_print_1(FILE **f1, FILE **f2) {
    if (!*f1) *f1 = fopen("ClientsDB.txt", "r");
    if (!*f2) *f2 = fopen("TransactionsDB.txt", "r");
    if (!*f1 || !*f2) {
        printf("Print 1: Database files could not be opened.\n");
        return;
    }

    char client_line[512], transaction_line[512];
    char cid[50], name[100], country[50], year[20], cid_check[50];

    rewind(*f1);
    while (fgets(client_line, sizeof(client_line), *f1)) {
        get_field_value(client_line, 0, cid);
        get_field_value(client_line, 1, name);
        get_field_value(client_line, 4, country); 
        get_field_value(client_line, 6, year);    

        printf("ID: %s\nFull Name: %s\nCountry: %s\nSince: %s\nRecent Transactions:\n", cid, name, country, year);

        rewind(*f2);
        int count = 0;
        while (fgets(transaction_line, sizeof(transaction_line), *f2) && count < 10) {
            get_field_value(transaction_line, 1, cid_check);
            if (strcmp(cid, cid_check) == 0) {
                transaction_line[strcspn(transaction_line, "\n")] = '\0';
                printf("\t-> %s\n", transaction_line); 
                count++;
            }
        }
        printf("\n");
    }
    rewind(*f1); rewind(*f2); 
}

void cmd_print_2(char ***clients, int *clients_count, char ***transactions, int *transactions_count) {
    char client_cid[50], transaction_cid[50];

    if (*clients == NULL || *clients_count == 0) {
        printf("Print 2: Arrays are empty.\n");
        return;
    }
    for (int i = 0; i < *clients_count; i++) {
        if (!(*clients)[i]) continue;
        get_field_value((*clients)[i], 0, client_cid);
        
        printf("--- CLIENT OVERVIEW ---\nRAW: %s\nHistory:\n", (*clients)[i]);

        if (*transactions != NULL && *transactions_count > 0) {
            for (int j = 0; j < *transactions_count; j++) {
                if (!(*transactions)[j]) continue;
                get_field_value((*transactions)[j], 1, transaction_cid);
                if (strcmp(client_cid, transaction_cid) == 0) {
                    printf("\t[TX] %s\n", (*transactions)[j]);
                }
            }
        }
        printf("\n"); 
    }
}

void cmd_print_3(CLIENT **clients) {
    CLIENT *tmp;
    TRANSACTION *tr;

    if(!clients || !*clients){
        printf("Print 3: Linked list is empty.\n");
        return;
    }

    tmp = *clients;

    while(tmp){
        printf("CLIENT ID: %s\nName: %s\nCountry: %s\nLTV: %.2f\n", 
               tmp->client_id, tmp->full_name, tmp->country_code, tmp->lifetime_value);

        tr = tmp->transactions;
        printf("Transactions:\n");
        while(tr){
            printf("\t%s | %s %.2f | %s | %s | %lld\n",
                   tr->transaction_id, tr->currency, tr->amount, 
                   tr->payment_method, tr->status, tr->timestamp);

            tr = tr->next;
        }

        printf("\n");
        tmp = tmp->next;
    }
}

void cmd_export(FILE **f2) {
    char file_line[MAX_LINE_LEN];
    char target_cid[50];
    int found = 0;
    FILE *vystup;

    if (!*f2) {
        printf("Export: Transaction database not opened.\n");
        return;
    }

    printf("Enter Client ID to export: ");
    scanf("%s", target_cid);
    
    rewind(*f2);

    while(fgets(file_line, sizeof(file_line), *f2)) {
        if (strstr(file_line, target_cid) != NULL) {
            found = 1; 
            vystup = fopen("Exported_Transactions.txt", "a"); 
            if (!vystup) {
                printf("Export: Failed to open export file.\n");
                return;
            }
            fprintf(vystup, "%s", file_line);
            fclose(vystup);
        }
    }
    if(found) printf("Export: Successfully exported records.\n");
    else printf("Export: No records found for ID %s.\n", target_cid);
}

void cmd_load_arrays(FILE **f1, FILE **f2, char ***clients, int *clients_count, char ***transactions, int *transactions_count) {
    char **nev;
    char client[LINE_MAX], transaction[LINE_MAX];

    if (!*f1 || !*f2) {
        printf("Load Arrays: Files are not initialized. Run 'print 1' first.\n");
        return;
    }
    rewind(*f1); rewind(*f2);

    free_rows(clients, *clients_count); *clients = NULL; *clients_count = 0;
    free_rows(transactions, *transactions_count); *transactions = NULL; *transactions_count = 0;
    while (fgets(client, sizeof(client), *f1)) {
        client[strcspn(client, "\n")] = '\0'; 
        nev = (char**)realloc(*clients, (*clients_count + 1) * sizeof(char *)); 
        if (!nev) {
            printf("Load Arrays: Critical memory error.\n");
            return;
        }
        *clients = nev;

        (*clients)[*clients_count] = (char*)malloc((strlen(client) + 1) * sizeof(char));
        if (!(*clients)[*clients_count]) {
            printf("Load Arrays: Critical memory error.\n");
            return;
        }

        strcpy((*clients)[*clients_count], client);
        (*clients_count)++;
    }

    while (fgets(transaction, sizeof(transaction), *f2)) {
        transaction[strcspn(transaction, "\n")] = '\0';
        nev = (char **)realloc(*transactions, (*transactions_count + 1) * sizeof(char *));
        if (!nev) {
            printf("Load Arrays: Critical memory error.\n");
            return;
        }

        *transactions = nev;
        (*transactions)[*transactions_count] = (char *)malloc((strlen(transaction) + 1) * sizeof(char));
        if (!(*transactions)[*transactions_count]) {
            printf("Load Arrays: Critical memory error.\n");
            return;
        }

        strcpy((*transactions)[*transactions_count], transaction);
        (*transactions_count)++;
    }
    printf("Load Arrays: Loaded %d clients and %d transactions into dynamic arrays.\n", *clients_count, *transactions_count);
}

void cmd_insert_array(char ***clients, int *clients_count, char ***transactions, int *transactions_count) {
    char transaction_id[37], client_id[37], currency[4], payment_method[20], status[15];
    double amount;
    long long timestamp;
    char buf[500];
    int index;

    if(*clients == NULL || *clients_count == 0) {
        printf("Insert Array: Data structures not initialized. Run 'load_arrays' first.\n");
        return;
    }
    
    printf("Enter insertion index: ");
    scanf("%d", &index);

    if(index < 0) index = 0;
    if (index > *transactions_count) index = *transactions_count;

    scan_data(transaction_id, client_id, &amount, currency, payment_method, status, &timestamp);
    sprintf(buf, "%s#%s#%.2f#%s#%s#%s#%lld", transaction_id, client_id, amount, currency, payment_method, status, timestamp);

    if(!push(transactions, transactions_count, index, buf)) {
        printf("Insert Array: Unexpected memory allocation error.\n");
        return;
    }
    printf("Insert Array: Transaction injected successfully.\n");
}

void cmd_delete_array(char ***clients, char ***transactions, int *clients_count, int *transactions_count) {
    int removed;
    char cid[50];

    if(!*clients || *clients_count == 0 || !*transactions || *transactions_count == 0) {
        printf("Delete Array: Data structures not initialized.\n");
        return;
    }
    printf("Enter Client ID to wipe transactions: ");
    scanf("%s", cid);
    removed = pop(transactions, transactions_count, cid);
    printf("Delete Array: %d transactions wiped from memory.\n", removed);
}

void cmd_load_lists(FILE *f1, FILE *f2, CLIENT **clients) {
    char line[512];
    int count_clients = 0, less = 0;
    CLIENT *check;

    if(!f1 || !f2) {
        printf("Load Lists: Database files not opened.\n");
        return;
    }

    if(clients) free_clients(clients);

    rewind(f1); 
    rewind(f2);

    while (fgets(line, sizeof(line), f1)) {
        line[strcspn(line, "\n")] = '\0';
        CLIENT *new_client = (CLIENT *)malloc(sizeof(CLIENT));
        if (!new_client) {
            printf("Load Lists: Memory allocation failed.\n");
            free_clients(clients);
            return;
        }
        new_client->transactions = NULL;
        new_client->next = NULL;

        char *p = strtok(line, "#");
        if(p) strncpy(new_client->client_id, p, sizeof(new_client->client_id));
        p = strtok(NULL, "#");
        if(p) strncpy(new_client->full_name, p, sizeof(new_client->full_name));
        p = strtok(NULL, "#");
        if(p) strncpy(new_client->email, p, sizeof(new_client->email));
        p = strtok(NULL, "#");
        if(p) strncpy(new_client->phone_number, p, sizeof(new_client->phone_number));
        p = strtok(NULL, "#");
        if(p) strncpy(new_client->country_code, p, sizeof(new_client->country_code));
        p = strtok(NULL, "#");
        if(p) new_client->lifetime_value = atof(p);
        p = strtok(NULL, "#");
        if(p) new_client->registration_date = atoll(p);

        if (!*clients) {
            *clients = new_client;
        } else {
            CLIENT *tmp = *clients;
            while (tmp->next) tmp = tmp->next;
            tmp->next = new_client;
        }
        count_clients++;
    }

    while (fgets(line, sizeof(line), f2)) {

        line[strcspn(line, "\n")] = '\0';

        char *p = strtok(line, "#");
        char tx_id[37] = ""; if(p) strncpy(tx_id, p, sizeof(tx_id));
        p = strtok(NULL, "#");
        char c_id[37] = ""; if(p) strncpy(c_id, p, sizeof(c_id));
        p = strtok(NULL, "#");
        double amt = 0.0; if(p) amt = atof(p);
        p = strtok(NULL, "#");
        char curr[4] = ""; if(p) strncpy(curr, p, sizeof(curr));
        p = strtok(NULL, "#");
        char pm[20] = ""; if(p) strncpy(pm, p, sizeof(pm));
        p = strtok(NULL, "#");
        char stat[15] = ""; if(p) strncpy(stat, p, sizeof(stat));
        p = strtok(NULL, "#");
        long long ts = 0; if(p) ts = atoll(p);

        CLIENT *target = *clients;
        while(target && strcmp(target->client_id, c_id) != 0) {
            target = target->next;
        }

        if(target) {
            TRANSACTION *new_tr = (TRANSACTION *)malloc(sizeof(TRANSACTION));
            if(!new_tr) {
                free_clients(clients);
                return;
            }

            strncpy(new_tr->transaction_id, tx_id, sizeof(new_tr->transaction_id));
            strncpy(new_tr->client_id, c_id, sizeof(new_tr->client_id));
            new_tr->amount = amt;
            strncpy(new_tr->currency, curr, sizeof(new_tr->currency));
            strncpy(new_tr->payment_method, pm, sizeof(new_tr->payment_method));
            strncpy(new_tr->status, stat, sizeof(new_tr->status));
            new_tr->timestamp = ts;
            new_tr->next = NULL;

            if(!target->transactions) {
                target->transactions = new_tr;
            } else {
                TRANSACTION *last = target->transactions;
                while(last->next) last = last->next;
                last->next = new_tr;
            }
        }
    }

    check = *clients;
    while(check != NULL) {
        if(strlen(check->full_name) == 0) less++;
        check = check->next;
    }
    printf("Load Lists: Successfully mapped %d corporate entities.\n", count_clients - less);
}

void cmd_insert_list(int pos, CLIENT **clients) {
    CLIENT *new_client, *prev, *cur;
    char c;
    char name[100], country[10], year[20], uuid[37];

    while((c = getchar()) != '\n' && c != EOF) {
        continue;
    };
    
    printf("Enter Full Name: ");
    fgets(name, 100, stdin); 
    name[strcspn(name, "\n")] = '\0'; 

    printf("Enter Country Code: "); scanf("%9s", country);
    printf("Enter Reg. Year (Unix Epoch): "); scanf("%19s", year);
    printf("Enter new UUID: "); scanf("%36s", uuid);

    cur = *clients;
    while(cur) {
        if(check_client(name, year, cur)) {
            printf("Insert List: Record duplication detected. Aborting.\n");
            return;
        }
        cur = cur->next;
    }

    new_client = (CLIENT *)malloc(sizeof(CLIENT));
    if(!new_client) return;

    strncpy(new_client->client_id, uuid, sizeof(new_client->client_id)-1);
    strncpy(new_client->full_name, name, sizeof(new_client->full_name)-1);
    strncpy(new_client->country_code, country, sizeof(new_client->country_code)-1);
    new_client->registration_date = atoll(year);
    new_client->lifetime_value = 0.0; 
    
    new_client->transactions = NULL;
    new_client->next = NULL;

    if(pos <= 1 || !*clients){
        new_client->next = *clients;
        *clients = new_client;
        printf("Insert List: Client integrated at head position.\n");
        return;
    }
    prev = *clients;
    int index = 1;

    while(prev->next && index < pos - 1) {
        prev = prev->next;
        index++;
    }
    
    new_client->next = prev->next;
    prev->next = new_client;
    printf("Insert List: Client integrated at position %d.\n", index + 1);
}

void cmd_delete_list(CLIENT **clients) {
    CLIENT *cl;
    TRANSACTION *tr, *removed;
    int deleted = 0;
    char target_cid[50];

    if (!clients || !*clients) {
        printf("Delete List: Linked list is empty.\n");
        return;
    }

    printf("Enter Client ID to purge: ");
    scanf("%s", target_cid);

    cl = *clients;
    while (cl) {
        tr = cl->transactions;
        while (tr && strcmp(tr->client_id, target_cid) == 0) {
            removed = tr;
            tr = tr->next;
            free(removed);
            deleted++;
        }
        cl->transactions = tr; 

        while(tr) {
            if (tr->next && strcmp(tr->next->client_id, target_cid) == 0) {
                removed = tr->next;
                tr->next = tr->next->next; 
                free(removed);
                deleted++;
            } else {
                tr = tr->next; 
            }
        }
        cl = cl->next;
    }
    printf("Delete List: %d transactions neutralized.\n", deleted);
}

void cmd_sort(CLIENT **clients) {
    CLIENT *tmp;
    TRANSACTION *sorted, *end, *max_prev, *max, *prev, *cur;

    if (!clients || !*clients) {
        printf("Sort: Linked list is empty.\n");
        return;
    }
    
    tmp = *clients; 
    while(tmp) {
        if (!tmp->transactions) {
            tmp = tmp->next;
            continue;
        }

        sorted = NULL;
        end = NULL;

        while(tmp->transactions) {
            max_prev = NULL; 
            max = tmp->transactions;
            prev = tmp->transactions;
            cur = tmp->transactions->next; 

            /* Sort descending by amount */
            while(cur) {
                if(cur->amount > max->amount) { 
                    max_prev = prev;
                    max = cur; 
                }
                prev = cur; 
                cur = cur->next;
            }

            if(max_prev) max_prev->next = max->next;
            else tmp->transactions = max->next;

            max->next = NULL;
            if (!sorted) {
                end = max; 
                sorted = end;
            } else {
                end->next = max; 
                end = max;
            }
        }
        tmp->transactions = sorted;
        tmp = tmp->next;
    }
    printf("Sort: Transactions successfully reordered by revenue weight (Descending).\n");
}

void cmd_exit(FILE **f1, FILE **f2, char ***clients, int *clients_count, char ***transactions, int *transactions_count, CLIENT **client_nodes) {
    if (f1 && *f1) fclose(*f1);
    if (f2 && *f2) fclose(*f2);
    if (f1) *f1 = NULL;
    if (f2) *f2 = NULL;

    free_rows(clients, *clients_count); *clients_count = 0;
    free_rows(transactions, *transactions_count); *transactions_count = 0;
    free_clients(client_nodes);
    printf("System: Assets liquidated. Environment secured. Goodbye.\n");
}

void print_help() {
    printf("\n--- CRM COMMAND MANUAL ---\n");
    printf("help           : Display this manual\n");
    printf("print 1        : Print raw data from DB files\n");
    printf("print 2        : Print client overview (Dynamic Arrays)\n");
    printf("print 3        : Print corporate profiles (Linked Lists)\n");
    printf("export         : Export client transactions to file\n");
    printf("load_arrays    : Load data into dynamic arrays\n");
    printf("insert_array   : Inject new transaction (Arrays)\n");
    printf("delete_array   : Wipe client transactions (Arrays)\n");
    printf("load_lists     : Load data into linked lists\n");
    printf("insert_list    : Integrate new client (Linked Lists)\n");
    printf("delete_list    : Purge client transactions (Linked Lists)\n");
    printf("sort           : Reorder transactions by revenue (Descending)\n");
    printf("exit           : Liquidate assets and secure environment\n");
    printf("--------------------------------------\n");
}

/* ===========================================================
   MAIN TERMINAL LOOP
   =========================================================== */
int main(void) {
    srand(time(NULL));

    char **clients = NULL, **transactions = NULL;
    int clients_count = 0, transactions_count = 0;
    CLIENT *client_nodes = NULL;

    char cmd[100], arg[100];
    FILE *f1 = NULL, *f2 = NULL;

    printf(">>> CRM TERMINAL INITIATED <<<\n");
    printf("Type 'help' for a list of available commands. " 
            "Warning: some of them must have initialized data structures)\n");

    while(1) {
        printf("\nENTER COMMAND> ");
        if (scanf("%s", cmd) != 1) continue;

        if (strcmp(cmd, "exit") == 0) {
            cmd_exit(&f1, &f2, &clients, &clients_count, &transactions, &transactions_count, &client_nodes);
            break;
        }
        else if (strcmp(cmd, "help") == 0) {
            print_help();
        }
        else if (strcmp(cmd, "print") == 0) {
            if (scanf("%s", arg) != 1) continue;
            if (strcmp(arg, "1") == 0) cmd_print_1(&f1, &f2);
            else if (strcmp(arg, "2") == 0) cmd_print_2(&clients, &clients_count, &transactions, &transactions_count);
            else if (strcmp(arg, "3") == 0) cmd_print_3(&client_nodes);
            else printf("System: Invalid print argument. Use 1, 2, or 3.\n");
        }
        else if (strcmp(cmd, "export") == 0) cmd_export(&f2);
        else if (strcmp(cmd, "load_arrays") == 0) cmd_load_arrays(&f1, &f2, &clients, &clients_count, &transactions, &transactions_count);
        else if (strcmp(cmd, "insert_array") == 0) cmd_insert_array(&clients, &clients_count, &transactions, &transactions_count);
        else if (strcmp(cmd, "delete_array") == 0) cmd_delete_array(&clients, &transactions, &clients_count, &transactions_count);
        else if (strcmp(cmd, "load_lists") == 0) cmd_load_lists(f1, f2, &client_nodes);
        else if (strcmp(cmd, "insert_list") == 0) {
            int pos;
            printf("Enter position: ");
            if (scanf("%d", &pos) != 1) continue;
            cmd_insert_list(pos, &client_nodes);
        }
        else if (strcmp(cmd, "delete_list") == 0) cmd_delete_list(&client_nodes);
        else if (strcmp(cmd, "sort") == 0) cmd_sort(&client_nodes);
        else {
            printf("System: Unrecognized command. Type 'help' for commands.\n");
        }
    }
    return 0;
}