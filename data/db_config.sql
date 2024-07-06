CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    password VARCHAR(50) NOT NULL
);

-- CREATE TABLE IF NOT EXISTS messages (
--     id SERIAL PRIMARY KEY,
--     user_id INT REFERENCES users(id),
--     message TEXT NOT NULL,
--     timestamp TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
-- );

PREPARE authenticate_user (VARCHAR, VARCHAR) AS
    SELECT * FROM users WHERE username = $1 AND password = $2;

PREPARE register_user (VARCHAR, VARCHAR) AS
    INSERT INTO users (username, password) VALUES ($1, $2);

-- PREPARE identification_session (VARCHAR) AS
--     INSERT INTO
