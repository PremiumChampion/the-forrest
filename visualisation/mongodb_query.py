from pymongo import MongoClient

# Requires the PyMongo package.
# https://api.mongodb.com/python/current

client = MongoClient('connection-string')
result = client['TheForrest']['TestData'].aggregate([
    {
        '$match': {
            'server_time': {
                '$exists': True
            }, 
            'data': {
                '$exists': True
            }
        }
    }, {
        '$sort': {
            'server_time': -1
        }
    }, {
        '$unwind': '$data'
    }, {
        '$project': {
            'id': '$data.id', 
            'server_time': 1, 
            'timestamp': '$data.t', 
            'humidity': '$data.mV_1', 
            'battery': '$data.mV_2'
        }
    }
])