walk( if type == "object" then with_entries( .key |= sub( "^_+"; "") ) else . end )