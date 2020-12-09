#include <stdio.h>
#include <stdlib.h>

typedef struct GridSpriteLite{
  int direction;
  int gridX, gridY;
  float partMove;
  int nextDirection;
};

typedef union CliSprite {
  int cli_addr;
  struct GridSpriteLite sprite;
};

char* stringify(struct GridSpriteLite* sprite, int* cli_addr) {
  size_t len = 0;
  char* struct_str = "%d={direction=%d,gridX=%d,gridY=%d,partMove=%f,nextDirection=%d}";

  len = snprintf(NULL, len, struct_str, *cli_addr,
    sprite->direction, sprite->gridX, sprite->gridY, sprite->partMove, sprite->nextDirection
  );

  char* str = calloc(1, sizeof *str * len + 1);
  if (!str) printf(stderr, "%s() error: memory allocation failed.\n", __func__);

  if (snprintf(str, len + 1, struct_str, *cli_addr,
    sprite->direction, sprite->gridX, sprite->gridY, sprite->partMove, sprite->nextDirection
  ) > len + 1) {
    printf(stderr, "%s() error: snprintf returned truncated result.\n", __func__);
    return NULL;
  }

  return str;
}

union CliSprite spritify(char* str) {
  int cli_addr[1], direction[1], gridX[1], gridY[1], nextDirection[1];
  float partMove[1];

  sscanf(str, "%d={direction=%d,gridX=%d,gridY=%d,partMove=%f,nextDirection=%d}",
  cli_addr, direction, gridX, gridY, partMove, nextDirection);

  //printf("Sprite for client id %d was recieved with the following values: ", *cli_addr);
  union CliSprite cli_sprite;
  struct GridSpriteLite gsl = {.direction=*direction, .gridX=*gridX,
    .gridY=*gridY, .partMove=*partMove, .nextDirection=*nextDirection};
  cli_sprite.cli_addr = *cli_addr;
  cli_sprite.sprite = gsl;

  printf("CLI ADR IN SPRITIFY: %d\n", cli_sprite.cli_addr);

  return cli_sprite;
}

int main() {
  struct GridSpriteLite sprite = {4, 6, 8, 0.05, 2};
  int* cli_addr = 6969;

  char* str = stringify(&sprite, &cli_addr);

  printf("Stringified struct: %s\n", str);

  union CliSprite u = spritify(str);

  printf("Sprite for client %d recieved the following values: \n", u.cli_addr);
  printf("direction=%d\n", u.sprite.direction);
  printf("gridX=%d\n", u.sprite.gridX);
  printf("gridY=%d\n", u.sprite.gridY);
  printf("partMove=%f\n", u.sprite.partMove);
  printf("nextDirection=%d\n", u.sprite.nextDirection);


  return 1;
}
