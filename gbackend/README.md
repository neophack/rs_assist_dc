# installation
pip install flask-restful

# using this test
pip install requests  


# run the server
python flask_simple_api.py

# test GET api
access http://127.0.0.1:5000/simple_api/ in a browser
you should see:
{
    "info": "This is a simple api for wrapping calculation."
}


# test POST api
change the image filepath in test_flask_simple_api.py
then run 
python test_flask_simple_api.py
You should see something like this:

image data len: 432322
Response:
{u'image_height': 1024, u'image_width': 681}


