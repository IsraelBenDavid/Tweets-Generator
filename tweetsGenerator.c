#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/************ Constant Variables ************/

#define MAX_WORDS_IN_SENTENCE_GENERATION 20
#define MAX_WORD_LENGTH 100
#define MAX_SENTENCE_LENGTH 1000
#define FULL_STOP '.'
#define TRUE 1
#define FALSE 0
#define STRTOL_BASE 10
#define ARGUMENTS_NEEDED 4
#define ARGUMENTS_OPTIONAL 5

/************ Messages to the user ************/

#define TWEET_FOREWORD "Tweet %d: "
#define USAGE_MSG "Usage: <seed> <Number of sentences to generate>" \
                      " <Path to file> <Optional - Number of words to read>\n"
#define SEED_ERROR_MSG "Error: seed must be number\n"
#define NUM_OF_TWEETS_ERROR_MSG "Error: number of tweets must be number\n"
#define FILE_ERROR_MSG "Error: could not open the file\n"
#define NUM_OF_WORDS_ERROR_MSG "Error: number of words must be number\n"
#define ALLOCATION_ERROR_MSG "Allocation Failure: allocation has failed\n"

typedef struct WordStruct {
  char *word;
  struct WordProbability *prob_list;
  int prob_size;
  int is_end;
  int occurrences;
} WordStruct;

typedef struct WordProbability {
  struct WordStruct *word_struct_ptr;
  int occurrences;
} WordProbability;

/************ LINKED LIST ************/
typedef struct Node {
  WordStruct *data;
  struct Node *next;
} Node;

typedef struct LinkList {
  Node *first;
  Node *last;
  int size;
} LinkList;

/************ Helper Functions ************/

/**
 * A function that receives a character and returns TRUE if it is a number.
 * Otherwise the function returns FALSE.
 * */
int is_num (char num)
{
  if (num < '0' || num > '9')
    {
      return FALSE;
    }
  return TRUE;
}

/**
 * A function that receives a pointer to string and returns TRUE if the string
 * contains only numbers.
 * Otherwise the function returns FALSE.
 */
int is_num_array (char *data)
{
  while (*data != '\0' && *data != '\n')
    {
      if (is_num (*data) == FALSE)
        {
          return FALSE;
        }
      data++;
    }
  return TRUE;
}

/************ Dictionary Filling ************/

/**
 * This function receives two WordStruct and returns a pointer to the
 * WordProbability object if the second WordStruct is found as
 * the first's prob_list
 * @param word_struct - search inside this WordStruct's prob_list
 * @param to_search - look for this WordStruct
 * @return - a pointer to the relevant WordProbability, NULL otherwise
 */
WordProbability *
get_from_probability_list (WordStruct *word_struct, WordStruct *to_search)
{
  if (word_struct->prob_list == NULL)
    {
      return NULL;
    }
  char *word_to_search = to_search->word;
  for (int i = 0; i < word_struct->prob_size; i++)
    {
      char *word_in_prob = word_struct->prob_list[i].word_struct_ptr->word;
      if (strcmp (word_to_search, word_in_prob) == 0)
        {
          return &(word_struct->prob_list[i]);
        }
    }
  return NULL;
}

/**
 * This function allocates memory for new prob_list of the given word struct
 * @param word_struct  - the given word struct
 */
void create_new_probability_list (WordStruct *word_struct)
{
  word_struct->prob_list = malloc (sizeof (WordProbability));
  // check if allocation fail
  if (word_struct->prob_list == NULL)
    {
      printf (ALLOCATION_ERROR_MSG);
      exit (EXIT_FAILURE);
    }
}

/**
 * This function increases the memory of the prob list of the given word
 * struct by one place
 * @param word_struct - the given word struct
 */
void increase_probability_list (WordStruct *word_struct)
{
  // reallocate the memory
  WordProbability *tmp;
  tmp = realloc (word_struct->prob_list,
                 (word_struct->prob_size + 1)
                 * sizeof (WordProbability));
  // check if allocation fail
  if (tmp == NULL)
    {
      printf (ALLOCATION_ERROR_MSG);
      exit (EXIT_FAILURE);
    }
  word_struct->prob_list = tmp;
}

/**
 * Gets 2 WordStructs. If second_word in first_word's prob_list,
 * update the existing probability value.
 * Otherwise, add the second word to the prob_list of the first word.
 * @param first_word
 * @param second_word
 * @return 0 if already in list, 1 otherwise.
 */
int
add_word_to_probability_list (WordStruct *first_word, WordStruct *second_word)
{
  // if first word has full stop - no need to add
  if (first_word->is_end == TRUE)
    {
      return TRUE;
    }
  // search the word in the first's prob_list
  WordProbability *word_prob = get_from_probability_list (first_word,
                                                          second_word);
  if (word_prob != NULL)
    {
      word_prob->occurrences++;
      return FALSE;
    }
  if (first_word->prob_list == NULL)
    {
      create_new_probability_list (first_word);
    }
  else
    {
      increase_probability_list (first_word);
    }
  first_word->prob_list[first_word->prob_size] = (WordProbability) {
      second_word, 1};
  first_word->prob_size++;
  return TRUE;
}

/**
 * this function looks for a word in the dictionary, if found,
 * return its word struct, if not return NULL
 * @param dictionary - dictionary to look in
 * @param to_search - the word to search
 * @return pointer to WS if found, NULL otherwise
 */
WordStruct *get_word_struct_from_dict (LinkList *dictionary, char *to_search)
{
  Node *curr = dictionary->first;
  for (int i = 0; i < dictionary->size; i++)
    {
      if (strcmp (to_search, curr->data->word) == 0)
        {
          return curr->data;
        }
      curr = curr->next;
    }
  return NULL;
}

/**
 * Add data to new node at the end of the given link list.
 * @param link_list Link list to add data to
 * @param data pointer to dynamically allocated data
 * @return 0 on success, 1 otherwise
 */
int add (LinkList *link_list, WordStruct *data)
{
  Node *new_node = malloc (sizeof (Node));
  if (new_node == NULL)
    {
      return 1;
    }
  *new_node = (Node) {data, NULL};

  if (link_list->first == NULL)
    {
      link_list->first = new_node;
      link_list->last = new_node;
    }
  else
    {
      link_list->last->next = new_node;
      link_list->last = new_node;
    }

  link_list->size++;
  return 0;
}

/**
 * this function checks if the given word end's in full stop "." (Nekuda)
 * @param word - the word to check
 * @return TRUE if yes FALSE if not
 */
int is_full_stop (char *word)
{
  unsigned long last_index = strlen (word) - 1;
  if (word[last_index] == FULL_STOP)
    {
      return TRUE;
    }
  return FALSE;
}
/**
 * this function creates new word struct
 * @param dictionary dictionary to fill
 * @param new_word_struct pointer to create the new WS
 * @param word the actual word in string
 * @return pointer to the new word struct
 */
WordStruct *create_new_word_struct (WordStruct *new_word_struct, char *word)
{
  // allocate memory
  new_word_struct = (WordStruct *) malloc (sizeof (WordStruct));
  // check if allocation fail
  if (new_word_struct == NULL)
    {
      printf (ALLOCATION_ERROR_MSG);
      exit (EXIT_FAILURE);
    }
  char *word_to_insert = malloc (strlen (word) + 1);
  if (word_to_insert == NULL)
    {
      printf (ALLOCATION_ERROR_MSG);
      exit (EXIT_FAILURE);
    }
  word_to_insert = strcpy (word_to_insert, word);
  *new_word_struct = (WordStruct) {word_to_insert,
                                   NULL,
                                   0,
                                   is_full_stop (word),
                                   1};
  return new_word_struct;
}

/**
 * this function adds new word struct to the dictionary
 * @param dictionary dictionary to fill
 * @param prev The previous word added to the dict
 * @param new_word new word to add
 * @return
 */
int add_word_struct_to_dict (LinkList *dictionary, char *prev, char *new_word)
{
  // check if the word already in the dictionary
  WordStruct *new_word_struct = get_word_struct_from_dict (dictionary,
                                                           new_word);
  if (new_word_struct == NULL)
    {
      new_word_struct = create_new_word_struct (new_word_struct, new_word);
      if (add (dictionary, new_word_struct) == 1)
        {
          printf (ALLOCATION_ERROR_MSG);
          exit (EXIT_FAILURE);
        }
    }
  else
    {
      new_word_struct->occurrences++;
    }
  if (prev[0] != '\0')
    {
      WordStruct *prev_s = get_word_struct_from_dict (dictionary, prev);
      add_word_to_probability_list (prev_s, new_word_struct);
    }
  return TRUE;
}

/**
 * Read word from the given file. Add every unique word to the dictionary.
 * Also, at every iteration, update the prob_list of the previous word with
 * the value of the current word.
 * @param fp File pointer
 * @param words_to_read Number of words to read from file.
 *                      If value is bigger than the file's word count,
 *                      or if words_to_read == -1 than read entire file.
 * @param dictionary Empty dictionary to fill
 */
void fill_dictionary (FILE *fp, int words_to_read, LinkList *dictionary)
{
  char line_buffer[MAX_SENTENCE_LENGTH];
  char *current_word;
  char prev_word[MAX_WORD_LENGTH] = "\0";
  int flag = FALSE;

  while (fgets (line_buffer, MAX_SENTENCE_LENGTH, fp) != NULL &&
         flag == FALSE)
    {
      current_word = strtok (line_buffer, " \n");
      while (current_word != NULL)
        {
          // handle this word
          add_word_struct_to_dict (dictionary, prev_word, current_word);
          strcpy (prev_word, current_word); // save the previous word
          current_word = strtok (NULL, " \n"); // next word
          words_to_read--;
          // Finish reading the words if we have reached the required amount
          if (words_to_read == 0)
            {
              flag = TRUE;
              break;
            }
        }
    }
}


/************ Tweets Generating ************/

/**
 * Get random number between 0 and max_number [0, max_number).
 * @param max_number
 * @return Random number
 */
int get_random_number (int max_number)
{
  return rand () % max_number;
}

/**
 * This functions returns how many words in dictionary do not end in full stop.
 * @param dictionary
 * @return returns how many words in dictionary do not end in full stop
 */
int count_not_last_words (LinkList *dictionary)
{
  int counter = 0;
  if (dictionary->size == counter)
    {
      return counter;
    }
  Node *node = dictionary->first;
  for (int i = 0; i < dictionary->size; i++)
    {
      if (node->data->is_end == FALSE)
        {
          counter++;
        }
      node = node->next;
    }
  return counter;
}

/**
 * Choose randomly the next word from the given dictionary, drawn uniformly.
 * The function won't return a word that end's in full stop '.' (Nekuda).
 * @param dictionary Dictionary to choose a word from
 * @return WordStruct of the chosen word
 */
WordStruct *get_first_random_word (LinkList *dictionary)
{
  // get the random place
  int random_place = get_random_number (count_not_last_words (dictionary));
  random_place++; // to make the place in [1, max)
  Node *node = dictionary->first;
  int i = 0; // counter until the random place
  //Keep counting if the word is not an ending
  if (node->data->is_end == FALSE)
    {
      i++;
    }
  while (i < random_place)
    {
      node = node->next;
      //Keep counting if the word is not an ending
      if (node->data->is_end == FALSE)
        {
          i++;
        }
    }

  return node->data;
}

/**
 * Choose randomly the next word. Depend on it's occurrence frequency
 * in word_struct_ptr->WordProbability.
 * @param word_struct_ptr WordStruct to choose from
 * @return WordStruct of the chosen word
 */
WordStruct *get_next_random_word (WordStruct *word_struct_ptr)
{
  // get the random place
  int random_place = get_random_number (word_struct_ptr->occurrences);
  random_place++; // to make the place in [1, max)
  int counter = 0; // counter until the random place
  for (int i = 0; i < word_struct_ptr->prob_size; i++)
    {
      // add the occurrences to the counter.
      // more occurrences = bigger probability to get chosen
      counter += word_struct_ptr->prob_list[i].occurrences;
      if (counter >= random_place)
        {
          return word_struct_ptr->prob_list[i].word_struct_ptr;
        }
    }
  return NULL; // if not found
}

/**
 * Receive dictionary, generate and print to stdout random sentence out of it.
 * The sentence most have at least 2 words in it.
 * @param dictionary Dictionary to use
 * @return Amount of words in printed sentence
 */
int generate_sentence (LinkList *dictionary)
{
  int counter = 1; // counter for the words
  WordStruct *first_word = get_first_random_word (dictionary);
  printf ("%s ", first_word->word);

  WordStruct *next_word = get_next_random_word (first_word);
  printf ("%s", next_word->word);

  // run until full stop
  while (next_word->is_end == FALSE)
    {
      counter++;
      // stop if reached the maximum words
      if (counter == MAX_WORDS_IN_SENTENCE_GENERATION)
        {
          printf ("\n");
          return counter;
        }
      printf (" ");
      next_word = get_next_random_word (next_word);
      printf ("%s", next_word->word);

    }
  counter++;
  printf ("\n");
  return counter;
}

/************ Free Memory Functions ************/

/**
 * this function receives WordStruct and free all its memory allocation
 * @param word_struct to free
 */
void free_word_struct (WordStruct *word_struct)
{
  free (word_struct->word);
  if (word_struct->prob_list != NULL)
    {
      free (word_struct->prob_list);
    }
  word_struct->word = NULL;
  word_struct->prob_list = NULL;
  free (word_struct);
  word_struct = NULL;
}

/**
 * Free the given dictionary and all of it's content from memory.
 * @param dictionary Dictionary to free
 */
void free_dictionary (LinkList *dictionary)
{
  // go node by node
  while (dictionary->first != dictionary->last)
    {
      free_word_struct (dictionary->first->data);
      Node *current = dictionary->first;
      dictionary->first = dictionary->first->next;
      free (current);
      current = NULL;
    }
  free_word_struct (dictionary->first->data);
  free (dictionary->first);
  dictionary->first = NULL;
  dictionary->last = NULL;
  free (dictionary);
  dictionary = NULL;
}

/**
 *
 * @param seed for random
 * @param num_of_tweets to generate
 * @param fp file to read from
 * @param num_of_words to read from the file
 */
void
make_tweets (long int seed, long int num_of_tweets, FILE *fp,
             int num_of_words)
{
  srand (seed);
  LinkList *dictionary = malloc (sizeof (LinkList));
  // check if allocation fail
  if (dictionary == NULL)
    {
      printf (ALLOCATION_ERROR_MSG);
      exit (EXIT_FAILURE);
    }
  // create dictionary
  *dictionary = (LinkList) {NULL, NULL, 0};
  fill_dictionary (fp, num_of_words, dictionary);
  // generate tweets
  for (int i = 1; i <= num_of_tweets; i++)
    {
      printf (TWEET_FOREWORD, i);
      generate_sentence (dictionary);
    }
  free_dictionary (dictionary);
}

/**
 * This function gets the arguments from the main, validates them and passing
 * them to make_tweets
 * @param seed_str
 * @param num_of_tweets_str
 * @param path
 * @param num_of_words_str
 * @return True if all good, False otherwise
 */
int
handle_args (char *seed_str, char *num_of_tweets_str, char *path,
             char *num_of_words_str)
{
  long int seed, num_of_tweets;
  int num_of_words = -1;
  char *ptr;
  if (is_num_array (seed_str) == FALSE)
    {
      printf (SEED_ERROR_MSG);
      return FALSE;
    }
  if (is_num_array (num_of_tweets_str) == FALSE)
    {
      printf (NUM_OF_TWEETS_ERROR_MSG);
      return FALSE;
    }
  if (num_of_words_str != NULL)
    {
      if (is_num_array (num_of_words_str) == FALSE)
        {
          printf (NUM_OF_WORDS_ERROR_MSG);
          return FALSE;
        }
      num_of_words = (int) strtol (num_of_words_str, &ptr, STRTOL_BASE);
    }
  FILE *fp;

  fp = fopen (path, "r");
  // check if file opened successfully
  if (fp == NULL)
    {
      printf (FILE_ERROR_MSG);
      return FALSE;
    }
  // convert and pass
  seed = strtol (seed_str, &ptr, STRTOL_BASE);
  num_of_tweets = strtol (num_of_tweets_str, &ptr, STRTOL_BASE);
  make_tweets (seed, num_of_tweets, fp, num_of_words);
  fclose (fp);
  return TRUE;
}

/**
 * @param argc
 * @param argv 1) Seed
 *             2) Number of sentences to generate
 *             3) Path to file
 *             4) Optional - Number of words to read
 */
int main (int argc, char *argv[])
{
  int flag;
  if (argc == ARGUMENTS_NEEDED)
    {
      flag = handle_args (argv[1], argv[2], argv[3], NULL);
    }
  else if (argc == ARGUMENTS_OPTIONAL)
    {
      flag = handle_args (argv[1], argv[2], argv[3], argv[4]);
    }
  else
    {
      printf (USAGE_MSG);
      return EXIT_FAILURE;
    }
  if (flag == FALSE)
    {
      return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;
}