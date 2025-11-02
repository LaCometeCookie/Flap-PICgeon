import pygame
import random
import sys
from enum import Enum
import serialComs as sc  # <<< IMPORT YOUR NEW SERIAL FILE


# --- Define Game States using Enum ---
class GameState(Enum):
    MENU = "menu"
    GAME = "game"
    SCORE_SCREEN = "score_screen"


# === 1. SERIAL CONNECTION (Happens FIRST) ===
# This will run the interactive CLI prompt before Pygame starts
sc.connect_to_serial_port()

# === 2. INITIALISATION ===
pygame.init()
WIDTH, HEIGHT = 600, 600
screen = pygame.display.set_mode((WIDTH, HEIGHT))
pygame.display.set_caption("Flap-PICGeon")
clock = pygame.time.Clock()
font = pygame.font.SysFont("Arial", 32)
small_font = pygame.font.SysFont("Arial", 25)
game_over_font = pygame.font.SysFont("Arial", 40)

# === 3. ASSETS (Loaded AFTER display is set) ===
bird_img = pygame.image.load("Sprites/flappybird.png").convert_alpha()
bird_img = pygame.transform.scale(bird_img, (46, 36))
logo_img = pygame.image.load("Sprites/Logo.png").convert_alpha()
logo_img = pygame.transform.scale(logo_img, (250, 100))
play_img = pygame.image.load("Sprites/Play.png").convert_alpha()
play_img = pygame.transform.scale(play_img, (120, 60))
score_img = pygame.image.load("Sprites/Score.png").convert_alpha()
score_img = pygame.transform.scale(score_img, (120, 60))
replay_img = pygame.Surface((60, 60))
replay_img.fill((255, 255, 255))

# === CONSTANTES PHYSIQUES ===
gravity = 0.25
flap_strength = -6.5
pipe_gap = 170
pipe_speed = 3
bg_far_speed = 1
bg_near_speed = 2
PIPE_WIDTH = 70

# === VARIABLES DU JEU ===
VITESSE = 1200  # ms entre chaque apparition de tuyau
bird_x = 80
bird_y = HEIGHT // 2
bird_velocity = 0
pipes = []
bg_far = []
bg_near = []
score = 0
best_score = 0
ground_y = HEIGHT - 80
game_active = True
bird_angle = 0.0
compteur = 0  # Your counter variable

# === COULEURS ===
SKY = (135, 206, 250)
GROUND = (139, 69, 19)
PIPE_BORDER_RED = (240, 80, 30)
PIPE_BORDER_OUTLINE = (160, 30, 0)
PIPE_RED = (220, 70, 30)
PIPE_BORDER = (160, 30, 0)
TEXT_COLOR = (255, 255, 255)
CITY_OUTLINE = (0, 0, 0)
CITY_BODY = (100, 100, 100)
MOUNTAIN_COLOR = (60, 180, 75)
TREE_COLOR = (0, 100, 0)
TRUNK_COLOR = (101, 67, 33)
CLOUD_COLOR = (255, 255, 255)
BLACK = (0, 0, 0)

# === ÉTATS DU JEU ===
game_state = GameState.MENU


# === FONCTIONS DU JEU ===
def create_pipe():
    height = random.randint(170, 430)
    return {"x": WIDTH, "height": height, "scored": False}


def move_pipes(pipes_list):
    for p in pipes_list:
        p["x"] -= pipe_speed
    return [p for p in pipes_list if p["x"] > -PIPE_WIDTH]


def draw_pipes(pipes_list):
    for p in pipes_list:
        pygame.draw.rect(screen, PIPE_RED, (p["x"] + 7, p["height"], 56, HEIGHT - p["height"]))
        pygame.draw.rect(screen, PIPE_BORDER, (p["x"] + 7, p["height"], 56, HEIGHT - p["height"]), 2)
        pygame.draw.rect(screen, PIPE_RED, (p["x"] + 7, 0, 56, p["height"] - pipe_gap))
        pygame.draw.rect(screen, PIPE_BORDER, (p["x"] + 7, 0, 56, p["height"] - pipe_gap), 2)
        pygame.draw.rect(screen, PIPE_BORDER_RED, (p["x"], p["height"], PIPE_WIDTH, 30))
        pygame.draw.rect(screen, PIPE_BORDER_OUTLINE, (p["x"], p["height"], PIPE_WIDTH, 30), 2)
        pygame.draw.rect(screen, PIPE_BORDER_RED, (p["x"], (p["height"] - pipe_gap - 30), PIPE_WIDTH, 30))
        pygame.draw.rect(screen, PIPE_BORDER_OUTLINE, (p["x"], (p["height"] - pipe_gap - 30), PIPE_WIDTH, 30), 2)


def draw_bird(x, y, vel):
    global bird_angle
    target_angle = max(-60, min(vel * -4, 60))
    bird_angle += (target_angle - bird_angle) * 0.2
    rotated_bird = pygame.transform.rotate(bird_img, bird_angle)
    rect = rotated_bird.get_rect(center=(x, y))
    screen.blit(rotated_bird, rect)


def check_collision(pipes_list):
    global game_active
    bird_center = (bird_x, bird_y)
    bird_radius = 15

    for p in pipes_list:
        top_rect = pygame.Rect(p["x"], 0, PIPE_WIDTH, p["height"] - pipe_gap)
        bottom_rect = pygame.Rect(p["x"], p["height"], PIPE_WIDTH, HEIGHT - p["height"])
        if circle_rect_collision(bird_center, bird_radius, top_rect) or \
                circle_rect_collision(bird_center, bird_radius, bottom_rect):
            game_active = False

    if bird_y + bird_radius >= ground_y or bird_y - bird_radius <= 0:
        game_active = False


def circle_rect_collision(circle_center, circle_radius, rect):
    cx, cy = circle_center
    closest_x = max(rect.left, min(cx, rect.right))
    closest_y = max(rect.top, min(cy, rect.bottom))
    dx = cx - closest_x
    dy = cy - closest_y
    return (dx * dx + dy * dy) < (circle_radius * circle_radius)


def draw_ground():
    pygame.draw.rect(screen, GROUND, (0, ground_y, WIDTH, HEIGHT - ground_y))


def display_score(current_score):
    text = font.render(f"Score: {current_score}", True, TEXT_COLOR)
    screen.blit(text, (10, 10))


# === DÉCOR GÉOMÉTRIQUE ===
def create_background_block(layer="far"):
    if layer == "far":
        block_type = random.choice(["mountains", "city", "clouds", "mountain_tree", "clouds"])
    else:
        block_type = random.choice(["prairie", "mountain_tree", "mountains", "empty", "empty", "empty"])
    block = {"x": WIDTH, "type": block_type}
    if block_type == "city":
        block["buildings"] = [random.randint(60, 120) for _ in range(3)]
    return block


def move_background(blocks, speed):
    for b in blocks:
        b["x"] -= speed
    return [b for b in blocks if b["x"] > -200]


def draw_empty(x):
    pygame.draw.rect(screen, (100, 200, 100), (x, ground_y - 5, 200, 5))


def draw_mountains(x):
    base_y = ground_y
    mountain_width = 120
    heights = [100, 140, 110]
    for i, h in enumerate(heights):
        start_x = x + i * (mountain_width - 20)
        peak_x = start_x + mountain_width // 2
        peak_y = base_y - h
        end_x = start_x + mountain_width
        pygame.draw.polygon(screen, (34, 139, 34), [(start_x, base_y), (peak_x, peak_y + 20), (end_x, base_y)])


def draw_mountain_tree(x):
    draw_mountains(x)
    pygame.draw.rect(screen, TRUNK_COLOR, (x + 150, ground_y - 60, 10, 30))
    pygame.draw.circle(screen, TREE_COLOR, (x + 155, ground_y - 70), 15)
    pygame.draw.rect(screen, TRUNK_COLOR, (x + 115, ground_y - 30, 10, 30))
    pygame.draw.circle(screen, TREE_COLOR, (x + 120, ground_y - 40), 15)


def draw_city(x, heights=None):
    if heights is None:
        heights = [100, 100, 100]
    for i, height in enumerate(heights):
        bx = x + i * 50
        pygame.draw.rect(screen, CITY_OUTLINE, (bx, ground_y - height, 40, height), 2)
        pygame.draw.rect(screen, CITY_BODY, (bx + 2, ground_y - height + 2, 36, height - 4))
        for fx in range(bx + 6, bx + 35, 10):
            for fy in range(ground_y - height + 10, ground_y - 10, 20):
                pygame.draw.rect(screen, CITY_OUTLINE, (fx, fy, 5, 5))


def draw_clouds(x):
    pygame.draw.circle(screen, CLOUD_COLOR, (x, 100), 20)
    pygame.draw.circle(screen, CLOUD_COLOR, (x + 25, 95), 25)
    pygame.draw.circle(screen, CLOUD_COLOR, (x + 50, 100), 20)


def draw_prairie(x):
    pygame.draw.rect(screen, (80, 200, 80), (x, ground_y - 10, 200, 20))
    wood_color = (139, 69, 19)
    for i in range(x + 10, x + 190, 20):
        pygame.draw.rect(screen, wood_color, (i, ground_y - 25, 5, 15))
    pygame.draw.rect(screen, wood_color, (x + 5, ground_y - 20, 190, 3))
    pygame.draw.rect(screen, wood_color, (x + 5, ground_y - 15, 190, 3))


def draw_background(blocks):
    for b in blocks:
        if b["type"] == "mountains":
            draw_mountains(b["x"])
        elif b["type"] == "mountain_tree":
            draw_mountain_tree(b["x"])
        elif b["type"] == "city":
            draw_city(b["x"], b.get("buildings"))
        elif b["type"] == "clouds":
            draw_clouds(b["x"])
        elif b["type"] == "prairie":
            draw_prairie(b["x"])
        elif b["type"] == "empty":
            draw_empty(b["x"])


def reset_game():
    global pipes, bird_y, bird_velocity, score, game_active, bird_angle, compteur
    pipes = []
    bird_y = HEIGHT // 2
    bird_velocity = 0
    score = 0
    bird_angle = 0.0
    game_active = True
    compteur = 0  # Reset flap counter
    # Send score 0 to PIC on reset
    sc.sendToPic(score)  # <<< USES serial_comms


# === MENU ET ÉCRAN SCORE ===
def draw_menu():
    screen.fill(SKY)
    screen.blit(logo_img, (WIDTH // 2 - logo_img.get_width() // 2, 50))
    screen.blit(bird_img, (WIDTH // 2 - bird_img.get_width() // 2, 200))

    # Add text instruction for SPACE bar
    start_text = small_font.render("Press SPACE to Start", True, BLACK)
    screen.blit(start_text, (WIDTH // 2 - start_text.get_width() // 2, 300))

    # Draw buttons and return their rects for collision
    play_rect = screen.blit(play_img, (WIDTH // 2 - 60, 350))
    score_rect = screen.blit(score_img, (WIDTH // 2 - 60, 420))
    replay_rect = screen.blit(replay_img, (WIDTH // 2 - 30, 500))
    return play_rect, score_rect, replay_rect


def draw_score_screen():
    screen.fill(SKY)
    display_text = game_over_font.render(f"Best Score: {best_score}", True, BLACK)
    screen.blit(display_text,
                (WIDTH // 2 - display_text.get_width() // 2, HEIGHT // 2 - display_text.get_height() // 2))
    small_text = small_font.render("Press R to return", True, BLACK)
    screen.blit(small_text, (WIDTH // 2 - small_text.get_width() // 2, HEIGHT - 100))


""" === ÉVÉNEMENTS CYCLIQUES === """
SPAWNPIPE = pygame.USEREVENT
pygame.time.set_timer(SPAWNPIPE, VITESSE)
SPAWNBG_FAR = pygame.USEREVENT + 1
pygame.time.set_timer(SPAWNBG_FAR, 2800)
SPAWNBG_NEAR = pygame.USEREVENT + 2
pygame.time.set_timer(SPAWNBG_NEAR, 2000)

# === BOUCLE PRINCIPALE ===
while True:
    # === EVENT HANDLING ===
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            sc.close_serial()  # <<< USES serial_comms
            pygame.quit()
            sys.exit()

        # === INPUT CLAVIER ===
        if event.type == pygame.KEYDOWN:
            # Start game from menu
            if game_state == GameState.MENU and event.key == pygame.K_SPACE:
                reset_game()
                game_state = GameState.GAME

            # Flap in-game
            elif game_state == GameState.GAME and game_active and event.key == pygame.K_SPACE:
                bird_velocity = flap_strength

            # Restart from game over
            elif game_state == GameState.GAME and not game_active and event.key == pygame.K_r:
                if score > best_score:
                    best_score = score
                reset_game()  # Reset game directly
                game_state = GameState.GAME  # Stay in GAME state

            # Return from score screen
            elif game_state == GameState.SCORE_SCREEN and event.key == pygame.K_r:
                game_state = GameState.MENU

        # === INPUT SOURIS MENU ===
        if game_state == GameState.MENU and event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            mx, my = pygame.mouse.get_pos()
            # Must call draw_menu to get rects, even though it will be drawn again
            play_rect, score_rect, _ = draw_menu()

            if play_rect.collidepoint(mx, my):
                reset_game()
                game_state = GameState.GAME
            elif score_rect.collidepoint(mx, my):
                game_state = GameState.SCORE_SCREEN

        # === SPAWN EVENTS ===
        if event.type == SPAWNPIPE and game_state == GameState.GAME and game_active:
            pipes.append(create_pipe())
        if event.type == SPAWNBG_FAR:
            bg_far.append(create_background_block("far"))
        if event.type == SPAWNBG_NEAR:
            bg_near.append(create_background_block("near"))

    # === SERIAL INPUT (PIC Controller) ===
    command = sc.read_serial_input()  # <<< USES serial_comms
    if command == "4" and game_state == GameState.GAME and game_active:
        compteur += 1
        print("Up=", compteur)
        bird_velocity = flap_strength

    # === LOGIQUE DU JEU ===
    if game_state == GameState.GAME and game_active:
        bird_velocity += gravity
        bird_y += bird_velocity
        pipes = move_pipes(pipes)
        bg_far = move_background(bg_far, bg_far_speed)
        bg_near = move_background(bg_near, bg_near_speed)
        check_collision(pipes)

        # --- Update Score & Send to PIC ---
        for p in pipes:  # Renamed 'ports' to 'p' to avoid confusion
            if not p.get("scored", False):
                if (p["x"] + PIPE_WIDTH) < bird_x:
                    score += 1
                    p["scored"] = True
                    sc.sendToPic(score)  # <<< USES serial_comms

    # === DESSIN SELON ÉTAT ===
    if game_state == GameState.MENU:
        draw_menu()
    elif game_state == GameState.SCORE_SCREEN:
        draw_score_screen()
    elif game_state == GameState.GAME:
        screen.fill(SKY)
        draw_background(bg_far)
        draw_background(bg_near)
        draw_pipes(pipes)
        draw_ground()
        draw_bird(bird_x, bird_y, bird_velocity)
        display_score(score)

        if not game_active:
            over_text = font.render("Press R to restart", True, (255, 50, 50))
            screen.blit(over_text, (WIDTH // 2 - over_text.get_width() // 2, HEIGHT // 2))

    # === UPDATE DISPLAY ===
    pygame.display.update()
    clock.tick(60)