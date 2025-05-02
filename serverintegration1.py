from flask import Flask, jsonify
from telegram import Bot, Update
from telegram.ext import Updater, MessageHandler, Filters, CallbackContext
from pydub import AudioSegment
from deepgram import Deepgram
import os
import string
from threading import Thread

# ============== Configuration ==============
TELEGRAM_TOKEN = "7806066942:AAE2JNHBo5y5LinNB781Wmf79YqG6SuNkAY"
DEEPGRAM_API_KEY = "56b7b14da2d4a3048c9b1d5e22072705fe9aca06"

app = Flask(__name__)
bot = Bot(token=TELEGRAM_TOKEN)
deepgram = Deepgram(DEEPGRAM_API_KEY)

latest_command = ""


# ============== Flask Endpoint ==============
@app.route("/get_command", methods=["GET"])
def get_command():
    global latest_command
    command = latest_command.rstrip(string.punctuation).strip()
    print(f"🛰 Sending to ESP32: {command}")
    latest_command = ""  # Clear after sending
    return jsonify({"command": command})


# ============== Deepgram Helper Function ==============
def transcribe_with_deepgram(wav_path):
    try:
        with open(wav_path, "rb") as audio:
            source = {"buffer": audio, "mimetype": "audio/wav"}
            response = deepgram.transcription.sync_prerecorded(source, {"punctuate": True})
            transcript = response['results']['channels'][0]['alternatives'][0].get('transcript', '')
            return transcript
    except Exception as e:
        print("🚨 Deepgram transcription error:", e)
        return ""


# ============== Command Cleaner ==============
def clean_command(text):
    # Remove trailing punctuation from the transcription (e.g., "Walk." -> "Walk")
    text = text.strip().rstrip(string.punctuation)

    # Step 1: Join adjacent single-character words (e.g., "P R" -> "PR")
    words = text.split()
    result = []
    buffer = ""

    for word in words:
        if len(word) == 1:
            buffer += word
        else:
            if buffer:
                result.append(buffer)
                buffer = ""
            result.append(word)
    if buffer:
        result.append(buffer)

    cleaned = ' '.join(result)

    # Step 2: Manual mode replacements
    replacements = {
        "Forward": "PF",
        "Backward": "PB",
        "Right": "PR",
        "Left": "PL",
        "Up": "PU",
        "Down": "PD",
        "Rotate right": "PRR",
        "Rotate left": "PRL",
        "Stop": "R"
    }

    for key, value in replacements.items():
        if cleaned.lower() == key.lower():
            print(f"🔁 Replaced '{key}' with '{value}'")
            return value

    return cleaned


# ============== Telegram Handlers ==============
def handle_voice(update: Update, context: CallbackContext):
    global latest_command
    user_id = update.message.from_user.id
    file = bot.getFile(update.message.voice.file_id)

    ogg_path = f"voice_{user_id}.ogg"
    wav_path = f"voice_{user_id}.wav"

    try:
        file.download(ogg_path)
        print(f"📥 Downloaded voice file: {ogg_path}")

        sound = AudioSegment.from_ogg(ogg_path)
        sound.export(wav_path, format="wav")
        print(f"🎵 Converted to WAV: {wav_path}")

        transcript = transcribe_with_deepgram(wav_path)

        os.remove(ogg_path)
        os.remove(wav_path)

        if transcript:
            cleaned_command = clean_command(transcript)
            latest_command = cleaned_command
            update.message.reply_text(f"🗣 Command received: {latest_command}")
            print(f"📥 New command saved: {latest_command}")
        else:
            update.message.reply_text("⚠️ Couldn't understand the voice.")
            print("❌ Empty transcript returned from Deepgram")

    except Exception as e:
        update.message.reply_text("❌ Voice processing failed.")
        print("🚨 Error processing voice message:", e)


def handle_text(update: Update, context: CallbackContext):
    global latest_command
    cleaned_command = clean_command(update.message.text)
    latest_command = cleaned_command
    update.message.reply_text(f"✅ Got it! Command: {latest_command}")
    print(f"📥 New text command saved: {latest_command}")


# ============== Telegram Bot Runner ==============
def run_bot():
    updater = Updater(token=TELEGRAM_TOKEN, use_context=True)
    dp = updater.dispatcher
    dp.add_handler(MessageHandler(Filters.voice, handle_voice))
    dp.add_handler(MessageHandler(Filters.text & ~Filters.command, handle_text))
    print("🤖 Telegram bot is running...")
    updater.start_polling()
    updater.idle()


# ============== Main Server Entry ==============
if __name__ == "__main__":
    Thread(target=run_bot).start()
    print("🚀 Flask server running at http://0.0.0.0:8000")
    app.run(host="0.0.0.0", port=8000)
