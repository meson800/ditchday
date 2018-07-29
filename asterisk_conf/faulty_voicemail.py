#!/usr/bin/env python3

import time
import agi as AGI
import random

def make_key(mailbox, key):
    """
    Given a mailbox as a number, and a key as a string,
    builds a string of the form <mailbox>_<key>
    for use in database functions
    """
    return str(mailbox) + '_' + key

def get_state(agi, tree, mailbox):
    """
    Given the AGI object, a sandbox tree 
    and a mailbox number, returns the current mailbox state 
    as an integer.
    """
    return int(agi.database_get(tree, make_key(mailbox, 'state')))

def write_state(agi, tree, mailbox, state):
    """
    Given the AGI object, a sandbox tree, a mailbox number,
    and a mailbox state as an int, writes this state into the
    database.
    """
    agi.database_put(tree, make_key(mailbox, 'state'), str(state))

def extract_share_id(digit_str):
    """
    Given a string of 6 digits, tries to extract
    a given mailbox ID. This is the second through
    fourth digits, taken mod 48
    """
    if len(digit_str) != 6:
        raise ValueError("Invalid share id length")
    # Check the checksum is ok
    digit_sum = sum([int(x) for x in digit_str[:-1]])

    if (digit_sum % 2) != int(digit_str[-1]):
        raise ValueError("Invalid checksum")

    return int(digit_str[1:4]) % 48

def generate_share_id(mailbox_id):
    """
    Generates a six-digit share ID as a string.

    The first digit is a random number, followed by three digits for
    the mailbox ID mod 48 (e.g., for a mailbox N, N + m * 48 encodes
    the same mailbox)

    The fifth digit is a random number, and the sixth digit
    is a checksum bit (0/1) based on the parity of the previous
    five numbers added together.
    """
    first_random_digit = random.randint(0,9)
    fifth_random_digit = random.randint(0,9)
    random_mailbox_num = mailbox_id + (48 * random.randint(0,17))
    
    pre_checksum = '{}{:03d}{}'.format(first_random_digit, random_mailbox_num, fifth_random_digit)

    checksum = sum([int(x) for x in pre_checksum]) % 2
    return pre_checksum + str(checksum)


agi = AGI.AGI()

#agi.say_alpha('hello')
digits = agi.get_data('dd/mailbox', timeout=1000, max_digits=3)
if len(digits) == 0:
    # Try again with silence, give them time to enter it
    digits = agi.get_data('silence/3', timeout=2000, max_digits=3)
if len(digits) == 0:
    agi.stream_file('dd/no_mailbox_entered')
    agi.stream_file('goodbye')
    exit()

sandbox_num = int(digits[0])
sandbox_tree = 'sb_' + str(sandbox_num)
mailbox_num = int(''.join(digits[1:]))

if (mailbox_num == 0):
    # Reset the given sandbox
    try:
        agi.database_deltree(sandbox_tree)
    except AGI.AGIDBError:
        pass
    # Re-init the puzzle
    agi.database_put(sandbox_tree, '2_pin', '7319')
    agi.database_put(sandbox_tree, '5_pin', '2442')

    agi.stream_file('dd/reset_sandbox')
    agi.say_digits([sandbox_num])
    agi.stream_file('goodbye')
    exit()

if (mailbox_num == 99):
    # Try to open a visitor session
    guest_id = agi.get_data('dd/enter_guest_id', timeout=5000, max_digits=6)
    if len(guest_id) == 0:
        # Try again with silence, give them time to enter it
        guest_id = agi.get_data('silence/3', timeout=5000, max_digits=6)
    if len(guest_id) == 0:
        agi.stream_file('goodbye')
        exit()

    try:
        share_id = extract_share_id(''.join(guest_id))
        # See if that mailbox is logged in
        if get_state(agi, sandbox_tree, share_id) >= 3:
            # Do voicemail stuff
            agi.stream_file('dd/valid_guest_id')
            if (share_id == 2):
                agi.stream_file('dd/voicemail')
                agi.stream_file('dd/message_1')
                agi.stream_file('dd/end_of_message')
            else:
                agi.stream_file('dd/no_voicemail')

            agi.stream_file('goodbye')
            exit()
    except ValueError:
        pass
    
    except AGI.AGIDBError:
        pass
    agi.stream_file('dd/invalid_guest')
    agi.stream_file('goodbye')
    exit()

# Now that we're here, we're actually trying to use a given mailbox!
# First, set the state to zero
write_state(agi, sandbox_tree, mailbox_num, 1)
# And clear the visitor tag
agi.database_del(sandbox_tree, make_key(mailbox_num, 'visitor'))

should_break = False
while should_break == False:
    # Give the main menu
    if get_state(agi, sandbox_tree, mailbox_num) == 3:
        result = agi.get_option('dd/main_auth_logged_in', escape_digits='0123456789*#')
    else:
        result = agi.get_option('dd/main_auth', escape_digits='0123456789*#')
    
    if result == '1':
        # Listen to voicemails
        # Check if we are validated
        if (get_state(agi, sandbox_tree, mailbox_num) != 3):
            agi.stream_file('dd/not_authenticated')
            continue
        
        if (mailbox_num == 2):
            agi.stream_file('dd/voicemail')
            agi.stream_file('dd/message_1')
            agi.stream_file('dd/end_of_message')
        else:
            agi.stream_file('dd/no_voicemail')

    if result == '3':
        # Guest listener
        if get_state(agi, sandbox_tree, mailbox_num) < 3:
            agi.stream_file('dd/guest_not_authenticated')
            continue

        # Check if we've already generated a key
        try:
            agi.database_get(sandbox_tree,
                make_key(mailbox_num, 'visitor'))
        except AGI.AGIDBError:
            # No key! Generate a share ID
            # Generate a share key
            share_id = generate_share_id(mailbox_num)

            agi.stream_file('dd/guest_id')
            agi.say_digits(share_id)

            agi.database_put(sandbox_tree,
                make_key(mailbox_num, 'visitor'),
                1)
            continue

        agi.stream_file('dd/guest_already_exists')

    if result == '4':
        # Update PIN
        new_state = get_state(agi, sandbox_tree, mailbox_num) + 4
        write_state(agi, sandbox_tree, mailbox_num, new_state)

        if new_state not in [5, 7]:
            agi.stream_file('dd/invalid_state')
            agi.say_digits([new_state])
            time.sleep(.5)
            agi.stream_file('goodbye')
            exit()

        if new_state == 5:
            agi.stream_file('dd/update_login_first')
            # Redirect to the login/logout interface
            result = '2'

        if new_state == 7:
            empty_pin = True
            while empty_pin == True:
                pin = agi.get_data('dd/enter_new_pin', timeout=2000, max_digits=4)
                if len(pin) == 0:
                    pin = agi.get_data('silence/3', timeout=2000, max_digits=4)

                if len(pin) == 4:
                    empty_pin = False

            new_pin_str = ''.join(pin)
            agi.stream_file('dd/new_pin_value')
            agi.say_digits(new_pin_str)
            agi.database_put(sandbox_tree,
                make_key(mailbox_num, 'pin'), pin)

        new_state = get_state(agi, sandbox_tree, mailbox_num) - 4
        write_state(agi, sandbox_tree, mailbox_num, new_state)


    if result == '2':
        # Authenticate/deauthenticate. Do this by incrementing state,
        # then checking where we are
        new_state = get_state(agi, sandbox_tree, mailbox_num) + 1
        write_state(agi, sandbox_tree, mailbox_num, new_state)

        if new_state not in [2, 4]:
            agi.stream_file('dd/invalid_state')
            agi.say_digits([new_state])
            time.sleep(.5)
            new_state = get_state(agi, sandbox_tree, mailbox_num) - 1
            write_state(agi, sandbox_tree, mailbox_num, new_state)
            agi.stream_file('goodbye')
            exit()


        if new_state == 2:
            # We are in the login phase
            empty_pin = True
            while empty_pin == True:
                pin = agi.get_data('dd/enter_pin', timeout=2000, max_digits=4)
                if len(pin) == 0:
                    pin = agi.get_data('silence/3', timeout=2000, max_digits=4)

                if len(pin) == 4:
                    empty_pin = False

            agi.stream_file('dd/processing')
            try:
                correct_pin = agi.database_get(sandbox_tree,
                    make_key(mailbox_num, 'pin'))
            except AGI.AGIDBError:
                # This must not be a valid mailbox
                time.sleep(1.45)
                agi.stream_file('dd/invalid_pin')
                new_state = get_state(agi, sandbox_tree, mailbox_num) - 1
                write_state(agi, sandbox_tree, mailbox_num, new_state)
                continue

            if ''.join(pin) != correct_pin:
                # Vulnerability 1
                # A timing attack is possible. Based on the number of
                # correct digits, we wait for a different amount of time
                time.sleep(.2)

                if len(pin) != 4:
                    # Just sleep for the entire 1.2 sec
                    time.sleep(1.2)
                else:
                    # Sleep for 500ms for each incorrect digit
                    for pair in zip(pin, list(correct_pin)):
                        if pair[0] != pair[1]:
                            time.sleep(.500)
                agi.stream_file('dd/invalid_pin')

                new_state = get_state(agi, sandbox_tree, mailbox_num) - 1
                write_state(agi, sandbox_tree, mailbox_num, new_state)
                continue

            else:
                # The correct pin was entered!
                agi.stream_file('dd/logged_in')
                new_state = get_state(agi, sandbox_tree, mailbox_num) + 1
                write_state(agi, sandbox_tree, mailbox_num, new_state)
                continue

        if new_state == 4:
            # We are in the logout phase
            decision = agi.get_option('dd/should_logout', escape_digits='12')

            if decision == '1':
                # Logout requested
                new_state = get_state(agi, sandbox_tree, mailbox_num) - 3
                write_state(agi, sandbox_tree, mailbox_num, new_state)
                continue

            # Stay authenticated (e.g. go back to state 3)
            new_state = get_state(agi, sandbox_tree, mailbox_num) - 1
            write_state(agi, sandbox_tree, mailbox_num, new_state)




    

agi.stream_file('goodbye')
