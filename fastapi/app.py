import os
from fastapi import FastAPI
from pydantic import BaseModel
from crewai import Agent, Task, Crew
from langchain_openai import ChatOpenAI

# === Konfiguracja LLM przez OpenRouter ===
llm = ChatOpenAI(
    model="openrouter/moonshotai/kimi-k2:free",
    api_key=os.getenv("OPENROUTER_API_KEY"),  # korzysta z Hugging Face Secret
    base_url="https://openrouter.ai/api/v1"
)

app = FastAPI(title="CrewAI + OpenRouter Example")

# --- Definicja agentów ---
guardian = Agent(
    role="Strażnik",
    goal="Analizuje dane z sensorów ESP32 i opisuje zdarzenie po polsku",
    backstory="Pilnuje pomieszczenia i tłumaczy dane techniczne na ludzki opis",
    llm=llm
)

analyst = Agent(
    role="Analityk",
    goal="Tworzy raport dzienny na podstawie logów zdarzeń",
    backstory="Specjalista od bezpieczeństwa",
    llm=llm
)

communicator = Agent(
    role="Komunikator",
    goal="Wysyła gotowe komunikaty do użytkownika",
    backstory="Asystent od komunikacji",
    llm=llm
)

# --- Definicja zadań ---

guardian_task = Task(
    description=(
        "Na podstawie danych sensorów PIR={pir}, distance={distance}, "
        "gyro=({gyro.x},{gyro.y},{gyro.z}), time={time}, "
        "opisz w 1 zdaniu co się stało."
    ),
    expected_output="Jednozdaniowy opis zdarzenia w języku polskim",
    agent=guardian
)

analyst_task = Task(
    description="Na podstawie listy zdarzeń utwórz raport dzienny w języku polskim.",
    expected_output="Raport dzienny w języku polskim",
    agent=analyst
)

communicator_task = Task(
    description="Przekaż wygenerowany komunikat do użytkownika.",
    expected_output="Gotowa wiadomość dla użytkownika w języku polskim",
    agent=communicator
)


crew = Crew(
    agents=[guardian, analyst, communicator],
    tasks=[guardian_task, analyst_task, communicator_task]
)

# --- MODELE API ---
class Gyro(BaseModel):
    x: float
    y: float
    z: float

class Event(BaseModel):
    pir: int
    distance: float
    time: str
    gyro: Gyro

# --- Endpoint ---
@app.post("/process")
def process_event(event: Event):
    result = crew.kickoff(inputs=event.dict())
    return {"result": result}

@app.get("/")
def health_check():
    return {"status": "ok"}