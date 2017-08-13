# Be sure to restart your server when you modify this file.

# Your secret key for verifying cookie session data integrity.
# If you change this key, all old sessions will become invalid!
# Make sure the secret is at least 30 characters and all random, 
# no regular words or you'll be exposed to dictionary attacks.
ActionController::Base.session = {
  :key         => '_rzb_ui_session',
  :secret      => '214bd86c820cbe7433942caf2a0220ac87798ed3a1399286bfe26d6783d428de72521c64f19ec3930972674af488c72e0992e93301b4b7bfba71fe7aaf55f5b3'
}

# Use the database for sessions instead of the cookie-based default,
# which shouldn't be used to store highly confidential information
# (create the session table with "rake db:sessions:create")
# ActionController::Base.session_store = :active_record_store
