import pygame
import random
import sys
import serialComs as SC  # <<< USES OUR UPDATED SERIAL FILE


# === 1. SERIAL CONNECTION (Happens FIRST) ===
SC.connect_to_serial_port()

# === 2. INITIALISATION ===
pygame.init()
WIDTH, HEIGHT = 600, 600
screen = pygame.display.set_mode((WIDTH, HEIGHT))
clock = pygame.time.Clock()
pygame.display.set_caption("Flap-PICGeon")

font = pygame.font.SysFont("Arial", 32)
small_font = pygame.font.SysFont("Arial", 25)
game_over_font = pygame.font.SysFont("Arial", 40)


""" ~~~~~~~~~~~~~~~~~~~~~ Chargement de l'image de l'oiseau ~~~~~~~~~~~~~~~~~~~~~ """
bird_img = pygame.image.load("Sprites/flappybird.png").convert_alpha()
bird_img = pygame.transform.scale(bird_img, (46, 36))  # taille ajustable

""" ~~~~~~~~~~~~~~~~~~~~~ Images du menu ~~~~~~~~~~~~~~~~~~~~~ """
logo_img = pygame.image.load("Sprites/Logo.png").convert_alpha()
play_img = pygame.image.load("Sprites/Play.png").convert_alpha()
score_img = pygame.image.load("Sprites/Score.png").convert_alpha()
replay_img = pygame.image.load("Sprites/Replay.png").convert_alpha()
# redimensionnement des éléments du menu
logo_img = pygame.transform.scale(logo_img, (345, 72))
play_img = pygame.transform.scale(play_img, (104, 58))  # 52*29
score_img = pygame.transform.scale(score_img, (104, 58))
replay_img = pygame.transform.scale(replay_img, (104, 58))

""" ~~~~~~~~~~~~~~~~~~~~~ Définition des Variables ~~~~~~~~~~~~~~~~~~~~~ """
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

compteur = 0
# SCORE screen (simple “line by line” fetch)
score_lines = [-1, -1, -1, -1]   # Button, Encoder, IR, Ultrasound
score_fetch = -1                         # -1 = idle; 0..3 = which line we’re filling



# === MÉMOIRE POUR LE MODE REPLAY ===
replay_mode = False           # True quand on rejoue une partie enregistrée
replay_start_time = 0         # Temps de début du replay
input_log = []                # Liste des inputs (temps relatifs)
pipe_log = []                 # Liste des tuyaux (x, hauteur, moment d'apparition)
recording = False             # True quand on enregistre une partie
replay_pipe_index = 0
replay_input_index = 0
speed_multiplier = 1


# === COULEURS ===
SKY = (135, 206, 250)
GROUND = (139, 69, 19)
PIPE_BORDER_RED = (240, 80, 30)
PIPE_BORDER_OUTLINE = (160, 30, 0)
PIPE_RED = (220, 70, 30)
PIPE_BORDER = (160, 30, 0)

BIRD_BODY = (255, 255, 0)
BIRD_BEAK = (255, 120, 0)
BIRD_EYE = (255, 255, 255)
BIRD_PUPIL = (0, 0, 0)
TEXT_COLOR = (255, 255, 255)
ARCADE_YELLOW = (23, 166, 76)
CITY_OUTLINE = (0, 0, 0)
CITY_BODY = (100, 100, 100)
MOUNTAIN_COLOR = (60, 180, 75)
TREE_COLOR = (0, 100, 0)
TRUNK_COLOR = (101, 67, 33)
CLOUD_COLOR = (255, 255, 255)
BLACK = (0, 0, 0)


# === ÉTATS DU JEU ===
STATE_MAIN_MENU = "main_menu"
STATE_MODE_SELECT = "mode_select"
STATE_GAME = "game"
STATE_SCORE_SCREEN = "score_screen"
game_state = STATE_MAIN_MENU


# === NEW: Menu and Mode Variables ===
menu_options = [
    "Space Bar (Python)",
    "Button (RA5)",
    "Encoder (RE0)",
    "IR Sensor (RE3)",
    "Ultrasound (RC2/RC3)"
]
selected_mode = 0   # Index of the menu_options list
game_mode = 0       # Stores the selected_mode *after* player hits Start
last_game_mode = 0

# PIC Mode IDs: 0=Button, 1=Encoder, 2=IR, 3=Ultrasound
pic_mode_map = [None, 0, 1, 2, 3]

# === FONCTIONS DU JEU ===
def create_pipe():
    height = random.randint(170, 430)
    return {"x": WIDTH, "height": height, "scored": False}


def move_pipes(pipes_list):
    for p in pipes_list:
        p["x"] -= pipe_speed * speed_multiplier
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


bird_angle = 0.0
def draw_bird(x, y, vel):
    global bird_angle
    target_angle = max(-60, min(vel * -4, 60))
    # interpolation plus rapide en mode replay, mais sans modifier l'amplitude
    interp_speed = 0.2 * speed_multiplier
    bird_angle += (target_angle - bird_angle) * interp_speed

    rotated_bird = pygame.transform.rotate(bird_img, bird_angle)
    rect = rotated_bird.get_rect(center=(x, y))
    screen.blit(rotated_bird, rect)


def check_collision(pipes_list):
    global game_active
    # Hitbox circulaire de rayon 15 px
    bird_center = (bird_x, bird_y)
    bird_radius = 15

    for p in pipes:
        top_rect = pygame.Rect(p["x"], 0, PIPE_WIDTH, p["height"] - pipe_gap)
        bottom_rect = pygame.Rect(p["x"], p["height"], PIPE_WIDTH, HEIGHT - p["height"])

        # collision avec les tuyaux (simple rect vs cercle)
        if circle_rect_collision(bird_center, bird_radius, top_rect) or \
                circle_rect_collision(bird_center, bird_radius, bottom_rect):
            game_active = False
            SC.send_game_over()

    # collision sol/plafond
    if bird_y + bird_radius >= ground_y or bird_y - bird_radius <= 0:
        game_active = False
        SC.send_game_over()


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


""" ~~~~~~~~~~~~~~~~~~~ FONCTIONS DE GESTION DU BACKGROUND ~~~~~~~~~~~~~~~~~~~~~ """
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
    pygame.draw.rect(screen, (101, 67, 33), (x + 150, ground_y - 60, 10, 30))
    pygame.draw.circle(screen, (0, 100, 0), (x + 155, ground_y - 70), 15)
    pygame.draw.rect(screen, (101, 67, 33), (x + 115, ground_y - 30, 10, 30))
    pygame.draw.circle(screen, (0, 100, 0), (x + 120, ground_y - 40), 15)


def draw_city(x, heights=None):
    if heights is None: heights = [100, 100, 100]
    for i, height in enumerate(heights):
        bx = x + i * 50
        pygame.draw.rect(screen, (0, 0, 0), (bx, ground_y - height, 40, height), 2)
        pygame.draw.rect(screen, (100, 100, 100), (bx + 2, ground_y - height + 2, 36, height - 4))
        for fx in range(bx + 6, bx + 35, 10):
            for fy in range(ground_y - height + 10, ground_y - 10, 20): pygame.draw.rect(screen, (0, 0, 0),
                                                                                         (fx, fy, 5, 5))


def draw_clouds(x):
    pygame.draw.circle(screen, (255, 255, 255), (x, 100), 20)
    pygame.draw.circle(screen, (255, 255, 255), (x + 25, 95), 25)
    pygame.draw.circle(screen, (255, 255, 255), (x + 50, 100), 20)


def draw_prairie(x):
    pygame.draw.rect(screen, (80, 200, 80), (x, ground_y - 10, 200, 20))
    wood_color = (139, 69, 19)
    for i in range(x + 10, x + 190, 20): pygame.draw.rect(screen, wood_color, (i, ground_y - 25, 5, 15))
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
    compteur = 0
    SC.send_live_score(score)


# === MENU ET ÉCRAN SCORE ===
def draw_main_menu():
    screen.fill(SKY)
    screen.blit(logo_img, (WIDTH // 2 - logo_img.get_width() // 2, 50))
    screen.blit(bird_img, (WIDTH // 2 - bird_img.get_width() // 2, 200))
    play_rect = screen.blit(play_img, (WIDTH // 2 - 60, 350))
    score_rect = screen.blit(score_img, (WIDTH // 2 - 60, 420))
    replay_rect = screen.blit(replay_img, (WIDTH // 2 - 60, 490))

    # === Texte clignotant "PRESS START" ===
    # Clignote toutes les 500 ms : visible quand tick//500 est pair
    current_time = pygame.time.get_ticks()
    if (current_time // 500) % 2 == 0:  # Calcul du temps pour afficher le texte
        press_start_text = game_over_font.render("PRESS START", True, ARCADE_YELLOW)
        screen.blit(
            press_start_text,
            (
                WIDTH // 2 - press_start_text.get_width() // 2,
                play_rect.top - 60  # 60 px au-dessus du bouton Play
            ),
        )

    return play_rect, score_rect, replay_rect


def draw_mode_select():
    global selected_mode
    screen.fill(SKY)
    screen.blit(logo_img, (WIDTH // 2 - logo_img.get_width() // 2, 50))

    for i, option in enumerate(menu_options):
        color = (255, 0, 0) if i == selected_mode else BLACK
        prefix = "> " if i == selected_mode else "  "
        text = font.render(prefix + option, True, color)
        screen.blit(text, (WIDTH // 2 - text.get_width() // 2, 250 + i * 40))

    start_text = font.render("Press SPACE to Start", True, BLACK)
    screen.blit(start_text, (WIDTH // 2 - start_text.get_width() // 2, 500))


def draw_score_screen():
    screen.fill(SKY)
    title = game_over_font.render("Best per Mode", True, BLACK)
    screen.blit(title, (WIDTH//2 - title.get_width()//2, 120))

    labels = ["Button (RA5)", "Encoder (RE0)", "IR Sensor (RE3)", "Ultrasound (RC2/RC3)"]
    y = 200
    for i, name in enumerate(labels):
        left = small_font.render(name, True, BLACK)
        screen.blit(left, (80, y))
        # show value or "..." while waiting
        val_text = "..." if score_lines[i] == -1 else str(score_lines[i])
        right = small_font.render(val_text, True, BLACK)
        screen.blit(right, (WIDTH - 80 - right.get_width(), y))
        y += 40

    hint = small_font.render("Press R to return", True, BLACK)
    screen.blit(hint, (WIDTH//2 - hint.get_width()//2, HEIGHT - 100))



""" === ÉVÉNEMENTS CYCLIQUES === """
SPAWNPIPE = pygame.USEREVENT
pygame.time.set_timer(SPAWNPIPE, VITESSE)
SPAWNBG_FAR = pygame.USEREVENT + 1
pygame.time.set_timer(SPAWNBG_FAR, 2800)
SPAWNBG_NEAR = pygame.USEREVENT + 2
pygame.time.set_timer(SPAWNBG_NEAR, 2000)

# === BOUCLE PRINCIPALE ===
while True:
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            sys.exit()

        # === INPUT CLAVIER ===
        if event.type == pygame.KEYDOWN:

            # --- MENU DE SÉLECTION DE MODE ---
            if game_state == STATE_MODE_SELECT:
                if event.key == pygame.K_UP:
                    selected_mode = (selected_mode - 1) % len(menu_options)
                elif event.key == pygame.K_DOWN:
                    selected_mode = (selected_mode + 1) % len(menu_options)
                elif event.key == pygame.K_SPACE:
                    game_mode = selected_mode
                    pic_mode = pic_mode_map[game_mode]
                    if pic_mode is not None:
                        SC.send_mode_select(pic_mode)
                        SC.send_select_slot(pic_mode)
                    reset_game()
                    game_state = STATE_GAME
                    recording = True  # Active l’enregistrement pour permettre le replay
                    replay_mode = False
                    input_log.clear()
                    pipe_log.clear()
                    replay_start_time = pygame.time.get_ticks()

            # --- EN JEU ---
            elif game_state == STATE_GAME and game_active:
                if game_mode == 0 and event.key == pygame.K_SPACE:
                    bird_velocity = flap_strength
                    # Enregistrement input si mode "record"
                    if recording:
                        input_log.append(pygame.time.get_ticks() - replay_start_time)

            # --- GAME OVER / RETOUR MENU ---
            elif game_state == STATE_GAME and not game_active and event.key == pygame.K_r:
                last_game_mode = game_mode
                if score > best_score:
                    best_score = score
                game_state = STATE_MAIN_MENU
                game_active = True
                replay_mode = False
                recording = False

            # --- ÉCRAN DU SCORE ---
            elif game_state == STATE_SCORE_SCREEN and event.key == pygame.K_r:
                game_state = STATE_MAIN_MENU

        # === CLICS DE SOURIS DANS LE MENU PRINCIPAL ===
        if game_state == STATE_MAIN_MENU and event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
            mx, my = pygame.mouse.get_pos()
            play_rect, score_rect, replay_rect = draw_main_menu()

            # --- Bouton PLAY ---
            if play_rect.collidepoint(mx, my):
                game_state = STATE_MODE_SELECT

            # --- Bouton SCORE ---
            elif score_rect.collidepoint(mx, my):
                # start a simple 0..3 chain: Button, Encoder, IR, Ultrasound
                score_lines[:] = [-1, -1, -1, -1]
                score_fetch = 0
                SC.send_select_slot(0)  # ask PIC for slot 0 first
                SC.send_request_best()  # PIC will reply CS:BEST,<n>
                game_state = STATE_SCORE_SCREEN


            # --- Bouton REPLAY ---
            elif replay_rect.collidepoint(mx, my):
                speed_multiplier = 2
                game_mode = last_game_mode # Reprendre le dernier mode sélectionné
                if pipe_log and input_log:
                    reset_game()
                    game_state = STATE_GAME
                    replay_mode = True
                    recording = False
                    replay_start_time = pygame.time.get_ticks()
                    replay_pipe_index = 0
                    replay_input_index = 0
                    pipes.clear()

                    # Construction des tuyaux fixes pour le replay
                    replay_pipe_spacing = 200
                    x_start = WIDTH + 100
                    pipes.clear()
                    for i, p_data in enumerate(pipe_log):
                        pipes.append({
                            "x": x_start + i * replay_pipe_spacing,
                            "height": p_data["height"],
                            "scored": False
                        })

        # === ÉVÉNEMENTS DE SPAWN ===
        if event.type == SPAWNPIPE and game_state == STATE_GAME and game_active and not replay_mode:
            new_pipe = create_pipe()
            pipes.append(new_pipe)
            if recording:
                pipe_log.append({
                    "time": pygame.time.get_ticks() - replay_start_time,
                    "height": new_pipe["height"]
                })

        if event.type == SPAWNBG_FAR:
            bg_far.append(create_background_block("far"))
        if event.type == SPAWNBG_NEAR:
            bg_near.append(create_background_block("near"))


    # ==========================================================
    # <<< UPDATED SERIAL PROTOCOL PARSER >>>
    # ==========================================================
    line = SC.read_serial_input()
    if line:
        # --- Handle Button Press ---
        if line == "CS:BTN,1":
            # Flap ONLY if a hardware mode (index > 0) is active
            if game_mode > 0 and game_state == STATE_GAME and game_active:
                compteur += 1
                print("Hardware Flap=", compteur)
                bird_velocity = flap_strength

        # --- Handle Best Score Report from PIC ---
        elif line.startswith("CS:BEST,"):
            try:
                new_best = int(line.split(',')[1])

                # If we are on the SCORE screen and fetching, fill one line then move on
                if game_state == STATE_SCORE_SCREEN and 0 <= score_fetch <= 3:
                    score_lines[score_fetch] = new_best
                    score_fetch += 1
                    if score_fetch <= 3:
                        # ask next slot (1=Encoder, 2=IR, 3=US)
                        SC.send_select_slot(score_fetch)
                        SC.send_request_best()
                else:
                    # legacy single value (just in case you show it elsewhere)
                    best_score = new_best

                print(f"PIC reported best: {new_best}")
            except Exception as e:
                print(f"Error parsing PIC command: {line} - {e}")


        # --- Handle Ready Signal ---
        elif line.startswith("CS:READY,"):
            print(f"PIC Controller is ready! Protocol={line.split(',')[1]}")
            SC.send_request_best()


    # === LOGIQUE DU JEU ===
    if game_state == STATE_GAME and game_active:
        # Vitesse et gravité ajustées selon le mode
        if replay_mode:
            speed_multiplier = 2
        else:
            speed_multiplier = 1

        bird_velocity += gravity * speed_multiplier
        bird_y += bird_velocity

        pipes = move_pipes(pipes)
        bg_far = move_background(bg_far, bg_far_speed)
        bg_near = move_background(bg_near, bg_near_speed)

        check_collision(pipes)

        # Calcul du score
        for p in pipes:
            if not p.get("scored", False) and (p["x"] + PIPE_WIDTH) < bird_x:
                score += 1
                p["scored"] = True
                SC.send_live_score(score)

        # Gestion du replay (entrées enregistrées)
        if replay_mode:
            elapsed = (pygame.time.get_ticks() - replay_start_time) * 2
            if replay_input_index < len(input_log) and elapsed >= input_log[replay_input_index]:
                bird_velocity = flap_strength * 1.1
                replay_input_index += 1


    # === DESSIN SELON ÉTAT ===
    if game_state == STATE_MAIN_MENU:
        draw_main_menu()

    elif game_state == STATE_MODE_SELECT:
        draw_mode_select()

    elif game_state == STATE_SCORE_SCREEN:
        draw_score_screen()

    elif game_state == STATE_GAME:
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

    pygame.display.update()
    clock.tick(60)
