import azure.functions as func
import logging
import os
from pymongo import MongoClient
import datetime

MongoDB_URI = os.environ["MONGODB_CONNECTION_STRING"]

app = func.FunctionApp()

@app.route(route="save_to_db", auth_level="anonymous", methods=["POST"])
def save_to_db(req: func.HttpRequest) -> func.HttpResponse:

    client = MongoClient(MongoDB_URI)
    # create db and collection if not exists
    db = client['TheForrest']
    collection = db['Writes']

    # insert current date and time and access ip
    collection.insert_one({"date": datetime.datetime.now(), "ip": req.headers.get('x-forwarded-for')})

    collection = db['TestData']

    try: 
        req_body = req.get_json() 
        collection.insert_one(req_body)
    except ValueError:
        req_text = req.get_body().decode('utf-8')
        collection.insert_one({"data": req_text})
        return func.HttpResponse("Invalid JSON", status_code=400) 
        pass 

    return func.HttpResponse(f"SUCCESS", status_code=200)