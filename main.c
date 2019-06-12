#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<math.h>

//PROBABILITIES
#define OFFSPRING_PCT                         0.50
#define MUTATION_PCT                          0.25
#define CROSSOVER_PROBABILITY                 0.50
#define SELECTION_PROBABILITY                 0.65
//PARAMETERS
#define MAXIMUM_HOURS_A_DAY     8
#define DAYS_A_WEEK             5
#define POPULATION_SIZE         1000
#define MAXIMUM_GENERATION      1000
#define OFFSPRING_POPULATION_SIZE           (int)(OFFSPRING_PCT * POPULATION_SIZE)
#define MUTANT_POPULATION_SIZE              (int)(MUTATION_PCT  * POPULATION_SIZE)
#define POPULATION_OUTPUT_SIZE  10


//COMPUTED PARAMETERS
#define ELEMENT_SIZE              MAXIMUM_HOURS_A_DAY * DAYS_A_WEEK
#define CROSSOVER_POPULATION_SIZE (int)(POPULATION_SIZE * CROSSOVER_PROBABILITY)
#define EXTERNAL_POPULATION_SIZE  (POPULATION_SIZE - (MUTANT_POPULATION_SIZE + CROSSOVER_POPULATION_SIZE))

//PENALTIES
#define LT_MIN_SESSION_LENGTH_PENALTY     0.0723
#define GT_MAX_SESSION_LENGTH_PENALTY     0.0345
#define EXCESS_OF_SESSIONS_PENALTY        0.0132
#define EXCESS_OF_SESSIONS_A_DAY_PENALTY  0.0517

//CONSTANTS
#define SUBJECT_LIST_SIZE        8

typedef struct _permutation_{
  int index;
  double value;
}t_permutation;

typedef struct _subject_{
    char* name;
    int   code;
    int   hours;
}t_subject;

typedef struct _element_{
  //an element of the population is an arrangement of subjects
  int value[ELEMENT_SIZE];
  double selection_p;
  double fitness;
  double subject_fitness[SUBJECT_LIST_SIZE];
}t_element;

typedef struct _parent_{
  t_element * element;
  int s_fittest_index;
  t_permutation ordered_subject[SUBJECT_LIST_SIZE];
}t_parent;

typedef int bool;
#define true  1
#define false 0

char* mock_subject_name[SUBJECT_LIST_SIZE] = {
  "algebra",
  "calculo",
  "programacion",
  "estadistica",
  "graficacion",
  "estructuras de datos",
  "administracion",
  "algebra lineal"
};

int mock_subject_code[SUBJECT_LIST_SIZE] = {
  1, 2, 2, 4, 5, 6, 7, 8
};

int mock_subject_hour[SUBJECT_LIST_SIZE] = {
  2, 4, 6, 8, 4, 2, 2, 2
};

char* DAY_NAME[DAYS_A_WEEK] = {
  "lunes", "martes", "miercoles", "jueves", "viernes"
};

//functions
void start_algorithm(void);

void initialise_reference_schedule(void);
void initialise_population();
void initialise_element(t_element * element);

void shuffle_element(t_element * element);

int evaluate_population(void);
void evaluate_element(t_element * element);
void evaluate_subject(t_element * element, int subject);
int  evaluate_day(t_element * element, const int subject, const int day);

void sort_over_fitness(void);

void crossover_population(void);
void crossover_elements(t_element * parent_a, t_element * parent_b, t_element * child);
t_element * choose_mate(void);

void mutate_population(void);
void mutate_element(t_element * element);

void sample_population(void);

void permute_element(t_element * element);
void permute_element_days(t_element * element);
void permute_element_hours_of_day(t_element * element);
void compute_element_selection_p(t_element * element);

void print_element_verbose(t_element * element);
void print_element(t_element * element);
void print_schedule_reference();
void print_population();

int selection_p_compare(const void * elem_a, const void * elem_b){
  double a = ((t_element *) elem_a)->fitness;
  double b = ((t_element *) elem_b)->fitness;
  return a < b ? -1 : a == b ? 0 : 1;
}

int permutation_compare(const void * elem_a, const void * elem_b){
  double a = ((t_permutation *) elem_a)->value;
  double b = ((t_permutation *) elem_b)->value;
  return a < b ? -1 : a == b ? 0 : 1;
}

//global variables
t_subject subject_list[SUBJECT_LIST_SIZE + 1];
int schedule_reference[ELEMENT_SIZE];
t_element population[POPULATION_SIZE];
t_element offspring[OFFSPRING_POPULATION_SIZE];
t_element mutants[MUTANT_POPULATION_SIZE];

int current_generation = 1;
double selection_p_sum;

int main () {
  srandom(time(NULL));
  start_algorithm();
  return 0;
}

void start_algorithm(void){
  int match = -1;
  initialise_reference_schedule();

  initialise_population();

  //game of life
  while(current_generation <= MAXIMUM_GENERATION && match == -1){
    //selection
    match = evaluate_population();
    if(match != -1){
      print_element_verbose(&population[match]);
      break;
    }

    sort_over_fitness();

    crossover_population();

    mutate_population();

    sample_population();

    /*for(int e = 0; e < POPULATION_SIZE; e++){
      printf("#%2d ", e);
      print_element(&population[e]);
    }*/
    if(current_generation % 100 == 0){
      printf("--------generation: %3d--------\n", current_generation);
    }
    current_generation++;
  //population ^= generation;
  //generation ^= population;
  //population ^=generation;
  }
  print_population();
}

void sample_population(){
  int e = 0;
  while(e < OFFSPRING_POPULATION_SIZE){
    population[e] = offspring[e];
    e++;
  }
  while(e < OFFSPRING_POPULATION_SIZE + MUTANT_POPULATION_SIZE){
    population[e] = mutants[e - OFFSPRING_POPULATION_SIZE];
    e++;
  }
  while(e < POPULATION_SIZE){
    shuffle_element(&population[e]);
    e++;
  }
}

int evaluate_population(){
  for(int e = POPULATION_SIZE - 1; e >= 0; e--){
    evaluate_element(&population[e]);
    //puts("evaluating");print_element(&population[e]);
    if(population[e].fitness == 0.0){
      return e;
    }
  }
  return -1;
}

void mutate_population(){
  //randomly select elements to be mutated
  //place candidates at the end of the array
  int r;
  for(int m = 0, e = POPULATION_SIZE - 1; m < MUTANT_POPULATION_SIZE; m++, e--){
    r = random() % e;
    //printf("m: %d\n", m);
    //save candidate into mutant population
    mutants[m] = population[r];
    //move selected element out of the population by replacing it with the las selectable element
    population[r] = population[e];
    //mutate element
    mutate_element(&mutants[m]);
    evaluate_element(&mutants[m]);
  }
}

void mutate_element(t_element * element){
    //select from 3 different types of mutation
    //puts("mutating");
    //day cycle
    //print_element(element);
    int next_cpy;
    int next_index, cur_index;
    for(int h = 0, rd = random() % DAYS_A_WEEK, cur_cpy = element->value[rd * MAXIMUM_HOURS_A_DAY]; h < MAXIMUM_HOURS_A_DAY; h++){
      cur_index = rd * MAXIMUM_HOURS_A_DAY + h;
      next_index = (rd * MAXIMUM_HOURS_A_DAY) + ((h + 1) % MAXIMUM_HOURS_A_DAY);
      //printf("cur: %2d, next: %2d\n", cur_index, next_index);
      next_cpy = element->value[next_index];
      element->value[next_index] = cur_cpy;
      cur_cpy = next_cpy;
    }
    //print_element(element);
}

void evaluate_element(t_element * element){
  element->fitness = 0.0;
  for(int s = 0; s < SUBJECT_LIST_SIZE; s++){
    element->subject_fitness[s] = 0.0;
    //restart fitness
    element->subject_fitness[s] = 0.0;
    evaluate_subject(element, s);
    //compute fitness
    element->fitness += element->subject_fitness[s];
  }
  //print_element(element);
}

void evaluate_subject(t_element * element, int subject){
  int number_of_blocks = 0;
  for(int d = 0; d < DAYS_A_WEEK; d++) {
    //printf("subject: %s\n", subject_list[subject].name);
    number_of_blocks += evaluate_day(element, subject, d);
  }
  if(number_of_blocks > 3){
    element->subject_fitness[subject] += EXCESS_OF_SESSIONS_PENALTY;
  }
  //print_element(element);
}
void penalty_element(t_element * element, int subject, int session_length, int number_of_blocks_a_day){
  if(session_length < 2){
    element->subject_fitness[subject] += LT_MIN_SESSION_LENGTH_PENALTY;
  }else if(session_length > 3){
    element->subject_fitness[subject] += GT_MAX_SESSION_LENGTH_PENALTY;
  }
  //printf("blocks a day: %d\n", number_of_blocks_a_day);
  if(number_of_blocks_a_day > 1){
    element->subject_fitness[subject] += EXCESS_OF_SESSIONS_A_DAY_PENALTY;
  }
}

int evaluate_day(t_element * element, const int subject, const int day){
  int session_length = 0;
  int number_of_blocks_a_day = 0;
  int h = 0, h_in;

  //printf("%s:\n", DAY_NAME[day]);
  //print_element(element);
  while(h < MAXIMUM_HOURS_A_DAY){
    session_length = 0;
    h_in = h;
    while( h_in < MAXIMUM_HOURS_A_DAY && subject == schedule_reference[element->value[day * MAXIMUM_HOURS_A_DAY + h_in]]){
      //printf("%2d,", day * MAXIMUM_HOURS_A_DAY + h_in);
      //printf(" %d,", element->value[day * MAXIMUM_HOURS_A_DAY + h_in]);
      //printf(" %d\n", schedule_reference[element->value[day * MAXIMUM_HOURS_A_DAY + h_in]]);
      session_length++;
      number_of_blocks_a_day++;
      h_in++;
    }

    if(h_in == h){
      //printf("%2d,", day * MAXIMUM_HOURS_A_DAY + h_in);
      //printf(" %d,", element->value[day * MAXIMUM_HOURS_A_DAY + h_in]);
      //printf(" %d\n", schedule_reference[element->value[day * MAXIMUM_HOURS_A_DAY + h_in]]);
      h++;
    }
    else h = h_in;
    //printf("session length: %d\n", session_length);
    penalty_element(element, subject, session_length, number_of_blocks_a_day);
  }
  return number_of_blocks_a_day;
}

void initialise_element(t_element * element){

  for(int e = ELEMENT_SIZE - 1; e >= 0; e--){
    element->value[e] = e;
  }

  element->fitness = 0.0;
  for(int s = SUBJECT_LIST_SIZE - 1; s >= 0; s--){
    element->subject_fitness[s] = 0.0;
  }
//  print_element(element);
  shuffle_element(element);
//  print_element(element);
}

void initialise_population(){
  //initialise population
  for(int e = POPULATION_SIZE - 1; e >= 0; e--){
    initialise_element(&population[e]);
    //print_element(&population[e]);
  }
}

void permute_element(t_element * element){

}
void permute_element_days(t_element * element){

}
void permute_element_hours_of_day(t_element * element){

}


void print_element(t_element * element){
  printf("ft: %2.6f\n", element->fitness);

  for(int h = 0; h < MAXIMUM_HOURS_A_DAY; h++){
    for(int d = 0; d < DAYS_A_WEEK; d++){
      printf("%2d ", element->value[d*MAXIMUM_HOURS_A_DAY + h]);
    }
    puts("");
  }
}

void print_population(){
  for(int b = 0; b < POPULATION_SIZE / POPULATION_OUTPUT_SIZE; b++){
    for(int e = b * POPULATION_OUTPUT_SIZE; e < (b + 1) * POPULATION_OUTPUT_SIZE; e++){
      printf("%2s=%2.5f=%6s", "", population[e].fitness, "");
    }
    printf("\n");
    for(int h = 0; h < MAXIMUM_HOURS_A_DAY; h++){
      for(int e = b * POPULATION_OUTPUT_SIZE; e < (b + 1) * POPULATION_OUTPUT_SIZE; e++){
        for(int d = 0; d < DAYS_A_WEEK; d++){
          printf("%2d ", population[e].value[d*MAXIMUM_HOURS_A_DAY + h]);
        }
        printf(" | ");
      }
      printf("\n");
    }
    printf("\n");
  }
  printf("--------offspring--------\n");
  for(int b = 0; b < OFFSPRING_POPULATION_SIZE / POPULATION_OUTPUT_SIZE; b++){
    for(int e = b * POPULATION_OUTPUT_SIZE; e < (b + 1) * POPULATION_OUTPUT_SIZE; e++){
      printf("%2s=%2.5f=%6s", "", offspring[e].fitness, "");
    }
    printf("\n");
    for(int h = 0; h < MAXIMUM_HOURS_A_DAY; h++){
      for(int e = b * POPULATION_OUTPUT_SIZE; e < (b + 1) * POPULATION_OUTPUT_SIZE; e++){
        for(int d = 0; d < DAYS_A_WEEK; d++){
          printf("%2d ", offspring[e].value[d*MAXIMUM_HOURS_A_DAY + h]);
        }
        printf(" | ");
      }
      printf("\n");
    }
    printf("\n");
  }
  printf("\n");
  printf("--------mutants--------\n");
  for(int b = 0; b < MUTANT_POPULATION_SIZE / POPULATION_OUTPUT_SIZE; b++){
    for(int e = b * POPULATION_OUTPUT_SIZE; e < (b + 1) * POPULATION_OUTPUT_SIZE; e++){
      printf("%2s=%2.5f=%6s", "", mutants[e].fitness, "");
    }
    printf("\n");
    for(int h = 0; h < MAXIMUM_HOURS_A_DAY; h++){
      for(int e = b*  MUTANT_POPULATION_SIZE; e < (b + 1) * MUTANT_POPULATION_SIZE; e++){
        for(int d = 0; d < DAYS_A_WEEK; d++){
          printf("%2d ", mutants[e].value[d*MAXIMUM_HOURS_A_DAY + h]);
        }
        printf(" | ");
      }
      printf("\n");
    }
    printf("\n");
  }
  printf("\n");
}

void print_element_verbose(t_element * element){
  printf("fitness: %f\n", element->fitness);
  for(int s = 0; s < SUBJECT_LIST_SIZE; s++){
    printf("%25s: %f\n", subject_list[s].name, element->subject_fitness[s]);
  }
  for(int d = 0; d < DAYS_A_WEEK; d++){
    printf("%28s", DAY_NAME[d]);
  }
  puts("");
  for(int h = 0; h < MAXIMUM_HOURS_A_DAY; h++){
    for(int d = 0; d < DAYS_A_WEEK; d++){
      printf("%25s(ref_idx %2d) ", subject_list[schedule_reference[element->value[d*MAXIMUM_HOURS_A_DAY + h]]].name, element->value[d*MAXIMUM_HOURS_A_DAY + h]);
    }
    puts("");
  }
}

void shuffle_element(t_element * element){
  //shuffle using fisher-yates
  //print_element(element);
  int x, value;
  for(int v = ELEMENT_SIZE - 1; v > 1; v--){
    x = (int) (random() % v);
    value = element->value[v];
    element->value[v] = element->value[x];
    element->value[x] = value;
  }
  //print_element(element);

}

void initialise_reference_schedule(){
  int total_hours = 0;
  //initialize subject references
  for(int i = SUBJECT_LIST_SIZE - 1; i >= 0; i--){
    subject_list[i].name  = mock_subject_name[i];
    subject_list[i].code  = mock_subject_code[i];
    subject_list[i].hours = mock_subject_hour[i];

    total_hours += subject_list[i].hours;
  }
  subject_list[SUBJECT_LIST_SIZE].name = "empty";
  subject_list[SUBJECT_LIST_SIZE].code = -1;
  subject_list[SUBJECT_LIST_SIZE].hours = ELEMENT_SIZE - total_hours;

  printf("total hours = %d\n", total_hours);
  printf("maximum hours = %d\n", ELEMENT_SIZE);
  printf("empty hours = %d\n", subject_list[SUBJECT_LIST_SIZE].hours);
  printf("population size:           %d\n", POPULATION_SIZE);
  printf("offspring population size: %d\n", OFFSPRING_POPULATION_SIZE);
  printf("mutant population size:    %d\n", MUTANT_POPULATION_SIZE);
  printf("external population size:  %d\n", EXTERNAL_POPULATION_SIZE);

  //initialize schedule references
  for(int s = 0, index = 0; s <= SUBJECT_LIST_SIZE; s++){
    for(int h = 0; h < subject_list[s].hours; h++, index++){
      schedule_reference[index] = s;
    }
  }
  print_schedule_reference();
}

void print_schedule_reference(){
  for(int d = 0; d < DAYS_A_WEEK; d++){
    for(int h = 0; h < MAXIMUM_HOURS_A_DAY; h++){
      printf("(%2d - %2d - %d)%s\n", d*MAXIMUM_HOURS_A_DAY+h, h, schedule_reference[d*MAXIMUM_HOURS_A_DAY + h], subject_list[schedule_reference[d*MAXIMUM_HOURS_A_DAY + h]].name);
    }
  }
}

void compute_element_selection_p(t_element * element){
  element->selection_p = exp(element->fitness);
  selection_p_sum += element->selection_p;
  //printf("%f\n", element->selection_p);
}

void crossover_population(){
  t_element * parent_a, * parent_b;

  for(int c = OFFSPRING_POPULATION_SIZE - 1; c >= 0; c--){
    parent_a = &population[c];
    //ensure different parents
    do{
      parent_b = choose_mate();
    }while(parent_a == parent_b);

    crossover_elements(parent_a, parent_b, &offspring[c]);
    evaluate_element(&offspring[c]);
  }
}

t_element * choose_mate(void){
  int index = random() % POPULATION_SIZE;
  return &population[index];
}

void crossover_elements(t_element * parent_a, t_element * parent_b, t_element * child){
  t_parent parents[2];
  t_parent * parent;
  t_permutation ordered_subject_fitness[SUBJECT_LIST_SIZE];
  bool set_subjects[SUBJECT_LIST_SIZE + 1] = {0};
  bool sessions[ELEMENT_SIZE];
  bool fit;
  int subject_index;

  //initialise parents
  parents[0].element = parent_a;
  parents[0].s_fittest_index = 0;
  parents[1].element = parent_b;
  parents[1].s_fittest_index = 0;
  //initialise child
  for(int e = 0; e < ELEMENT_SIZE; e++){
    child->value[e] = -1;
  }

  //order subjects_fitness of parents
  for(int p = 0; p < 2; p++){
    parent = &parents[p];
    for(int s = 0; s < SUBJECT_LIST_SIZE; s++){
      parent->ordered_subject[s].index = s;
      parent->ordered_subject[s].value = parent->element->subject_fitness[s];
    }
    qsort(parent->ordered_subject, SUBJECT_LIST_SIZE, sizeof(t_permutation), permutation_compare);
    //for(int s = 0; s < SUBJECT_LIST_SIZE; s++){
    //  printf("%2d - %f -> %s\n", parent->ordered_subject[s].index, parent->ordered_subject[s].value, subject_list[parent->ordered_subject[s].index].name);
    //}
    //print_element(parent->element);
  }
  //inherit process
  int p_index;
  do{
    //choose random parent to inherit a subject
    p_index = random() % 2;
    //printf("index parent: %d\n", p_index);
    parent = &parents[p_index];

    subject_index = parent->ordered_subject[parent->s_fittest_index].index;
    if(!set_subjects[subject_index]){
      //print_element(parent->element);
      //printf("fittest index: %d", parent->s_fittest_index);
      //print_element(parent->element);

      fit = true;
      //find subject occurences
      for(int e = 0; e < ELEMENT_SIZE; e++){
        //gather subject sessions
        if(schedule_reference[parent->element->value[e]] == subject_index){
          //printf("1 ");
          sessions[e] = true;
        }else {
          //printf("0 ");
          sessions[e] = false;
        }
        //if(e % MAXIMUM_HOURS_A_DAY == MAXIMUM_HOURS_A_DAY - 1) puts("");
        //do sessions fit inside offspring?
        if(sessions[e] && child->value[e] != -1){
          //does not fit, flag it
          fit = false;
          break;
        }
      }

      if(fit){
        //puts("fits");
        //insert session into child
        for(int e = 0; e < ELEMENT_SIZE; e++){
          if(sessions[e]){
            child->value[e] = parent->element->value[e];
            //printf("e: %2d; child: %d\n", e, child->value[e]);
          }
        }
        //set subject as inserted
        set_subjects[subject_index] = true;
      }
    }
    //move fittest index
    parent->s_fittest_index++;
  }while(parent->s_fittest_index < SUBJECT_LIST_SIZE);

  //puts("parent a");print_element(parents[0].element);
  //puts("parent b");print_element(parents[1].element);
  //randomly fill holes on child
  //puts("attempting completion of child");
  for(int s = 0, s_index = 0, free_index = 0; s <= SUBJECT_LIST_SIZE; s++){
    //printf("atempting to insert subject: %s, %2d hours\n", subject_list[s].name, subject_list[s].hours);
    if(!set_subjects[s]){
      //insert subject
      for(int h = 0; h < subject_list[s].hours; h++){
        //find first free space
        while(child->value[free_index] != -1){
          free_index++;
        }
        child->value[free_index] = s_index++;
        //printf("free space: %2d - %2d\n", free_index, child->value[free_index]);

      }
    }else{
      //puts("already set");//add hours to index and change to next subject references
      //for(int e = 0; e < ELEMENT_SIZE; e++){
      //  if(schedule_reference[child->value[e]] == s)printf("used space: %2d - %2d\n", e, child->value[e]);
      //}
      s_index += subject_list[s].hours;
    }
  }

  //print_element(parents[0].element);
  //print_element(child);
  //print_element(parents[1].element);
}

void sort_over_fitness(){
  qsort(population, POPULATION_SIZE, sizeof(t_element), &selection_p_compare);
}
