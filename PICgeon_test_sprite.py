import pygame
import random
import sys
#import serial
#ser = serial.Serial('COM3', 9600, timeout=0.01)


# === INITIALISATION ===
pygame.init()
WIDTH, HEIGHT = 600, 600
screen = pygame.display.set_mode((WIDTH, HEIGHT))
clock = pygame.time.Clock()
font = pygame.font.SysFont("Arial", 32)

""" ~~~~~~~~~~~~~~~~~~~~~ Chargement de l'image de l'oiseau ~~~~~~~~~~~~~~~~~~~~~ """
bird_img = pygame.image.load("flappybird.png").convert_alpha()
bird_img = pygame.transform.scale(bird_img, (46, 36))  # taille ajustable


# === CONSTANTES PHYSIQUES ===
gravity = 0.25
flap_strength = -6.5
pipe_gap = 150      # Espacement entre les tuyaux haut et bas
pipe_speed = 3
bg_far_speed = 1    # couche arrière (lente)
bg_near_speed = 2   # couche avant (rapide)

# Largeur logique du tuyau (utilisée pour dessin & score)
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
ground_y = HEIGHT - 80
game_active = True

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
CITY_OUTLINE = (0, 0, 0)
CITY_BODY = (100, 100, 100)
MOUNTAIN_COLOR = (60, 180, 75)
TREE_COLOR = (0, 100, 0)
TRUNK_COLOR = (101, 67, 33)
CLOUD_COLOR = (255, 255, 255)
BLACK = (0, 0, 0)

"""def read_serial_input():
    #--Lit les messages du microcontrôleur s’il y en a.
    if ser.in_waiting > 0:  # des données sont dispo ?
        line = ser.readline().decode(errors='ignore').strip()
        return line
    return None"""


# === FONCTIONS ===
def create_pipe():
    """Crée un tuyau aléatoire et initialise la clé 'scored' à False."""
    height = random.randint(170, 430)
    return {"x": WIDTH, "height": height, "scored": False}

def move_pipes(pipes):
    """Déplace les tuyaux."""
    for p in pipes:
        p["x"] -= pipe_speed
    return [p for p in pipes if p["x"] > -PIPE_WIDTH]

def draw_pipes(pipes):
    """Dessine les tuyaux."""

    for p in pipes:
        # On dessine le tube (intérieur) avec un léger offset comme dans ton code original
        pygame.draw.rect(screen, PIPE_RED, (p["x"]+7, p["height"], 56, HEIGHT - p["height"]))
        pygame.draw.rect(screen, PIPE_BORDER, (p["x"]+7, p["height"], 56, HEIGHT - p["height"]), 2)
        pygame.draw.rect(screen, PIPE_RED, (p["x"]+7, 0, 56, p["height"] - pipe_gap))
        pygame.draw.rect(screen, PIPE_BORDER, (p["x"]+7, 0, 56, p["height"] - pipe_gap), 2)
        # Haut du tuyau (bords)
        pygame.draw.rect(screen, PIPE_BORDER_RED, (p["x"], p["height"], PIPE_WIDTH, 30))
        pygame.draw.rect(screen, PIPE_BORDER_OUTLINE, (p["x"], p["height"], PIPE_WIDTH, 30), 2)
        pygame.draw.rect(screen, PIPE_BORDER_RED, (p["x"], (p["height"] - pipe_gap - 30), PIPE_WIDTH, 30))
        pygame.draw.rect(screen, PIPE_BORDER_OUTLINE, (p["x"], (p["height"] - pipe_gap - 30), PIPE_WIDTH, 30), 2)

bird_angle = 0.0  # angle initial de l'oiseau
def draw_bird(x, y, vel):
    global bird_angle
    target_angle = max(-60, min(vel * -4, 60))
    # interpolation douce entre l'angle actuel et la cible
    bird_angle += (target_angle - bird_angle) * 0.2

    rotated_bird = pygame.transform.rotate(bird_img, bird_angle)
    rect = rotated_bird.get_rect(center=(x, y))
    screen.blit(rotated_bird, rect)


def check_collision(pipes):
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

    # collision sol/plafond
    if bird_y + bird_radius >= ground_y or bird_y - bird_radius <= 0:
        game_active = False


def circle_rect_collision(circle_center, circle_radius, rect):
    """Renvoie True si un cercle touche un rectangle."""
    cx, cy = circle_center
    closest_x = max(rect.left, min(cx, rect.right))
    closest_y = max(rect.top, min(cy, rect.bottom))
    dx = cx - closest_x
    dy = cy - closest_y
    return (dx * dx + dy * dy) < (circle_radius * circle_radius)


def draw_ground():
    """Dessine le sol."""
    pygame.draw.rect(screen, GROUND, (0, ground_y, WIDTH, HEIGHT - ground_y))

def display_score(score):
    """Affiche le score."""
    text = font.render(f"Score: {score}", True, TEXT_COLOR)
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
    pygame.draw.rect(screen, (100, 200, 100), (x, ground_y-5, 200, 5))

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
    pygame.draw.rect(screen, TRUNK_COLOR, (x+150, ground_y-60, 10, 30))
    pygame.draw.circle(screen, TREE_COLOR, (x+155, ground_y-70), 15)
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
    pygame.draw.circle(screen, CLOUD_COLOR, (x+25, 95), 25)
    pygame.draw.circle(screen, CLOUD_COLOR, (x+50, 100), 20)

def draw_prairie(x):
    pygame.draw.rect(screen, (80, 200, 80), (x, ground_y-10, 200, 20))
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
        """~~~~~~~~~~~~~~~~~~ BARRE ESPACE INPUT ~~~~~~~~~~~~~~~~~~"""
        if event.type == pygame.KEYDOWN and game_active:
            if event.key == pygame.K_SPACE:
                bird_velocity = flap_strength
        if event.type == pygame.KEYDOWN and not game_active:
            if event.key == pygame.K_r:
                bird_y = HEIGHT // 2
                bird_velocity = 0
                pipes = []
                bg_far = []
                bg_near = []
                score = 0
                game_active = True
        if event.type == SPAWNPIPE and game_active:
            pipes.append(create_pipe())
        if event.type == SPAWNBG_FAR:
            bg_far.append(create_background_block("far"))
        if event.type == SPAWNBG_NEAR:
            bg_near.append(create_background_block("near"))

    # Ajouter ici les autres contrôles
    # --- lecture du port série USB
    #command = read_serial_input()
    #if command == "FLAP" and game_active:
    #    bird_velocity = flap_strength

    # === LOGIQUE DU JEU ===
    if game_active:
        bird_velocity += gravity
        bird_y += bird_velocity
        pipes = move_pipes(pipes)
        bg_far = move_background(bg_far, bg_far_speed)
        bg_near = move_background(bg_near, bg_near_speed)
        check_collision(pipes)
        # --- nouvelle logique de score : robustement détecte le franchissement
        for p in pipes:
            if not p.get("scored", False):
                # on incrémente le score quand la bordure droite du tuyau est passée
                # on utilise < et non == pour éviter les oublis dus aux sauts par pas
                if (p["x"] + PIPE_WIDTH) < bird_x:
                    score += 1
                    p["scored"] = True

    # === DESSIN ===
    screen.fill(SKY)
    # couche arrière (lente)
    draw_background(bg_far)
    # couche avant (rapide)
    draw_background(bg_near)
    # éléments de jeu
    draw_pipes(pipes)
    draw_ground()
    draw_bird(bird_x, bird_y, bird_velocity)
    display_score(score)

    if not game_active:
        over_text = font.render("Game Over! Press R", True, (255, 50, 50))
        screen.blit(over_text, (WIDTH//2 - over_text.get_width()//2, HEIGHT//2))

    pygame.display.update()
    clock.tick(60)
